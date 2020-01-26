#include "wm_server.h"

extern "C" {
  #include <unistd.h>
  #include <wayland-server-core.h>
  #include <wlr/backend.h>
  #include <wlr/render/wlr_renderer.h>
  #include <wlr/types/wlr_cursor.h>
  #include <wlr/types/wlr_compositor.h>
  #include <wlr/types/wlr_data_device.h>
  #include <wlr/types/wlr_input_device.h>
  #include <wlr/types/wlr_keyboard.h>
  #include <wlr/types/wlr_matrix.h>
  #include <wlr/types/wlr_output.h>
  #include <wlr/types/wlr_output_layout.h>
  #include <wlr/types/wlr_pointer.h>
  #include <wlr/types/wlr_seat.h>
  #include <wlr/types/wlr_xcursor_manager.h>
  #include <wlr/types/wlr_xdg_shell.h>
  #include <wlr/xwayland.h>
  #include <wlr/util/log.h>
  #include <wlr/backend/libinput.h>
  #include <xkbcommon/xkbcommon.h>
}

#include <iostream>

#include "wm_keyboard.h"
#include "wm_view.h"
#include "wm_view_xwayland.h"
#include "wm_view_xdg.h"
#include "wm_output.h"

wm_server::wm_server()
  : cursor_mode(wm_CURSOR_NONE) {

}

static void new_input_notify(wl_listener *listener, void *data) {
  /* This event is raised by the backend when a new input device becomes
   * available. */
  wm_server *server = wl_container_of(listener, server, new_input);
  auto device = static_cast<struct wlr_input_device *>(data);

  switch (device->type) {
  case WLR_INPUT_DEVICE_KEYBOARD:
    server->new_keyboard(device);
    break;

  case WLR_INPUT_DEVICE_POINTER:
    server->new_pointer(device);
    break;

  default:
    break;

  }
  /* We need to let the wlr_seat know what our capabilities are, which is
   * communiciated to the client. In TinyWL we always have a cursor, even if
   * there are no pointer devices, so we always include that capability. */
  uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
  if (!wl_list_empty(&server->keyboards)) {
    caps |= WL_SEAT_CAPABILITY_KEYBOARD;
  }

  wlr_seat_set_capabilities(server->seat, caps);
}

static void seat_request_cursor_notify(wl_listener *listener, void *data) {
  wm_server *server = wl_container_of(listener, server, request_cursor);
  /* This event is rasied by the seat when a client provides a cursor image */
  auto event = static_cast<wlr_seat_pointer_request_set_cursor_event*>(data);
  wlr_seat_client *focused_client = server->seat->pointer_state.focused_client;
  /* This can be sent by any client, so we check to make sure this one is
   * actually has pointer focus first. */
  if (focused_client == event->seat_client) {
    /* Once we've vetted the client, we can tell the cursor to use the
     * provided surface as the cursor image. It will set the hardware cursor
     * on the output that it's currently on and continue to do so as the
     * cursor moves between outputs. */
    wlr_cursor_set_surface(server->cursor, event->surface, event->hotspot_x, event->hotspot_y);
  }
}

static void cursor_motion_notify(wl_listener *listener, void *data) {
  /* This event is forwarded by the cursor when a pointer emits a _relative_
   * pointer motion event (i.e. a delta) */
  wm_server *server = wl_container_of(listener, server, cursor_motion);
  auto event = static_cast<struct wlr_event_pointer_motion*>(data);
  /* The cursor doesn't move unless we tell it to. The cursor automatically
   * handles constraining the motion to the output layout, as well as any
   * special configuration applied for the specific input device which
   * generated the event. You can pass NULL for the device if you want to move
   * the cursor around without any input. */
  wlr_cursor_move(server->cursor, event->device, event->delta_x, event->delta_y);
  server->process_cursor_motion(event->time_msec);
}

static void cursor_motion_absolute_notify(wl_listener *listener, void *data) {
  /* This event is forwarded by the cursor when a pointer emits an _absolute_
   * motion event, from 0..1 on each axis. This happens, for example, when
   * wlroots is running under a Wayland window rather than KMS+DRM, and you
   * move the mouse over the window. You could enter the window from any edge,
   * so we have to warp the mouse there. There is also some hardware which
   * emits these events. */
  wm_server *server = wl_container_of(listener, server, cursor_motion_absolute);
  auto event = static_cast<struct wlr_event_pointer_motion_absolute*>(data);
  wlr_cursor_warp_absolute(server->cursor, event->device, event->x, event->y);
  server->process_cursor_motion(event->time_msec);
}

static void cursor_button_notify(wl_listener *listener, void *data) {
  /* This event is forwarded by the cursor when a pointer emits a button
   * event. */
  wm_server *server = wl_container_of(listener, server, cursor_button);
  auto event = static_cast<struct wlr_event_pointer_button*>(data);
  /* Notify the client with pointer focus that a button press has occurred */
  wlr_seat_pointer_notify_button(server->seat, event->time_msec, event->button, event->state);

  double sx, sy;
  wlr_surface *surface;
  wm_view *view = server->desktop_view_at(server->cursor->x, server->cursor->y, &surface, &sx, &sy);

  if (event->state == WLR_BUTTON_RELEASED) {
    /* If you released any buttons, we exit interactive move/resize mode. */
    server->cursor_mode = wm_CURSOR_PASSTHROUGH;
  } else {
    if (view != NULL) {
      /* Focus that client if the button was _pressed_ */
      view->focus_view(surface);
    }
  }
}

static void cursor_axis_notify(wl_listener *listener, void *data) {
  /* This event is forwarded by the cursor when a pointer emits an axis event,
   * for example when you move the scroll wheel. */
  wm_server *server = wl_container_of(listener, server, cursor_axis);
  auto event = static_cast<wlr_event_pointer_axis*>(data);
  /* Notify the client with pointer focus of the axis event. */
  wlr_seat_pointer_notify_axis(server->seat, event->time_msec, event->orientation,
    event->delta, event->delta_discrete, event->source);
}

static void cursor_frame_notify(wl_listener *listener, void *data) {
  /* This event is forwarded by the cursor when a pointer emits an frame
   * event. Frame events are sent after regular pointer events to group
   * multiple events together. For instance, two axis events may happen at the
   * same time, in which case a frame event won't be sent in between. */
  wm_server *server = wl_container_of(listener, server, cursor_frame);
  /* Notify the client with pointer focus of the frame event. */
  wlr_seat_pointer_notify_frame(server->seat);
}

static void output_frame_notify(wl_listener *listener, void *data) {
  /* This function is called every time an output is ready to display a frame,
   * generally at the output's refresh rate (e.g. 60Hz). */
  wm_output *output = wl_container_of(listener, output, frame);
  output->render();
}

static void new_output_notify(wl_listener *listener, void *data) {
  /* This event is rasied by the backend when a new output (aka a display or
   * monitor) becomes available. */
  wm_server *server = wl_container_of(listener, server, new_output);
  auto wlr_output = static_cast<struct wlr_output*>(data);

  wlr_log(WLR_ERROR, "%s connected", wlr_output->name);

  /* Some backends don't have modes. DRM+KMS does, and we need to set a mode
   * before we can use the output. The mode is a tuple of (width, height,
   * refresh rate), and each monitor supports only a specific set of modes. We
   * just pick the monitor's preferred mode, a more sophisticated compositor
   * would let the user configure it. */
  if (!wl_list_empty(&wlr_output->modes)) {
    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
    wlr_output_set_mode(wlr_output, mode);
    wlr_output_enable(wlr_output, true);
    if (!wlr_output_commit(wlr_output)) {
      wlr_log(WLR_ERROR, "Failed to commit output");
      return;
    }
  }

  if (strcmp(wlr_output->name, "X11-1") == 0) {
    wlr_output_set_scale(wlr_output, 2);
  }

  if (strcmp(wlr_output->name, "eDP-1") == 0) {
    wlr_output_set_scale(wlr_output, 3);
  }

  if (strcmp(wlr_output->name, "DP-0") == 0
   || strcmp(wlr_output->name, "DP-1") == 0
   || strcmp(wlr_output->name, "DP-2") == 0) {
    wlr_output_set_scale(wlr_output, 1);
  }

  /* Allocates and configures our state for this output */
  wm_output *output = new wm_output();
  output->wlr_output = wlr_output;
  output->server = server;

  /* Sets up a listener for the frame notify event. */
  output->frame.notify = output_frame_notify;
  wl_signal_add(&wlr_output->events.frame, &output->frame);

  /* Adds this to the output layout. The add_auto function arranges outputs
   * from left-to-right in the order they appear. A more sophisticated
   * compositor would let the user configure the arrangement of outputs in the
   * layout. */
  wlr_output_layout_add_auto(server->output_layout, wlr_output);

  /* Creating the global adds a wl_output global to the display, which Wayland
   * clients can see to find out information about the output (such as
   * DPI, scale factor, manufacturer, etc). */
  wlr_output_create_global(wlr_output);

  wl_list_insert(&server->outputs, &output->link);
}

static void xdg_surface_map_notify(wl_listener *listener, void *data) {
  /* Called when the surface is mapped, or ready to display on-screen. */
  wm_view *view = wl_container_of(listener, view, map);
  wm_server *server = view->server;
  view->mapped = true;

  server->position_view(view);
  view->focus();
}

static void xdg_surface_unmap_notify(wl_listener *listener, void *data) {
  /* Called when the surface is unmapped, and should no longer be shown. */
  wm_view *view = wl_container_of(listener, view, unmap);
  view->mapped = false;
  wm_server *server = view->server;
  server->pop_view(view);
}

static void xdg_surface_destroy_notify(wl_listener *listener, void *data) {
  /* Called when the surface is destroyed and should never be shown again. */
  wm_view *view = wl_container_of(listener, view, destroy);
  wl_list_remove(&view->link);
  delete view;
}

static void xdg_toplevel_request_move_notify(wl_listener *listener, void *data) {
  /* This event is raised when a client would like to begin an interactive
   * move, typically because the user clicked on their client-side
   * decorations. Note that a more sophisticated compositor should check the
   * provied serial against a list of button press serials sent to this
   * client, to prevent the client from requesting this whenever they want. */
  wm_view *view = wl_container_of(listener, view, request_move);
  view->begin_interactive(wm_CURSOR_MOVE, 0);
}

static void xdg_toplevel_request_resize_notify(wl_listener *listener, void *data) {
  /* This event is raised when a client would like to begin an interactive
   * resize, typically because the user clicked on their client-side
   * decorations. Note that a more sophisticated compositor should check the
   * provied serial against a list of button press serials sent to this
   * client, to prevent the client from requesting this whenever they want. */
  auto event = static_cast<wlr_xdg_toplevel_resize_event*>(data);
  wm_view *view = wl_container_of(listener, view, request_resize);
  view->begin_interactive(wm_CURSOR_RESIZE, event->edges);
}

static void xdg_toplevel_request_maximize_notify(wl_listener *listener, void *data) {
  wm_view *view = wl_container_of(listener, view, request_maximize);
  view->toggle_maximize();
}

static void xwayland_surface_request_configure_notify(wl_listener *listener, void *data) {
  wm_view_xwayland *view = wl_container_of(listener, view, request_configure);
  auto event = static_cast<wlr_xwayland_surface_configure_event*>(data);
  view->x = event->x;
  view->y = event->y;
  wlr_xwayland_surface_configure(event->surface, event->x, event->y, event->width, event->height);
}

static void new_xwayland_surface_notify(wl_listener *listener, void *data) {
  wm_server *server = wl_container_of(listener, server, new_xwayland_surface);
  auto xwayland_surface = static_cast<wlr_xwayland_surface*>(data);

  wm_view_xwayland *view = new wm_view_xwayland();

  view->x = 100;
  view->y = 100;
  view->server = server;
  view->xwayland_surface = xwayland_surface;
  // view->surface_type = wm_XWAYLAND_SURFACE;

  view->map.notify = xdg_surface_map_notify;
  wl_signal_add(&xwayland_surface->events.map, &view->map);

  view->unmap.notify = xdg_surface_unmap_notify;
  wl_signal_add(&xwayland_surface->events.unmap, &view->unmap);

  view->destroy.notify = xdg_surface_destroy_notify;
  wl_signal_add(&xwayland_surface->events.destroy, &view->destroy);

  view->request_configure.notify = xwayland_surface_request_configure_notify;
  wl_signal_add(&xwayland_surface->events.request_configure, &view->request_configure);

  view->request_move.notify = xdg_toplevel_request_move_notify;
  wl_signal_add(&xwayland_surface->events.request_move, &view->request_move);

  view->request_resize.notify = xdg_toplevel_request_resize_notify;
  wl_signal_add(&xwayland_surface->events.request_resize, &view->request_resize);

  view->request_maximize.notify = xdg_toplevel_request_maximize_notify;
  wl_signal_add(&xwayland_surface->events.request_maximize, &view->request_maximize);

   /* Add it to the list of views. */
  wl_list_insert(&server->views, &view->link);
}

static void new_xdg_surface_notify(wl_listener *listener, void *data) {
  /* This event is raised when wlr_xdg_shell receives a new xdg surface from a
   * client, either a toplevel (application window) or popup. */
  wm_server *server = wl_container_of(listener, server, new_xdg_surface);
  auto xdg_surface = static_cast<wlr_xdg_surface*>(data);
  if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
    return;
  }

  /* Allocate a wm_view for this surface */
  wm_view_xdg *view = new wm_view_xdg();
  view->x = 0;
  view->y = 0;
  view->server = server;
  view->xdg_surface = xdg_surface;
  // view->surface_type = wm_XDG_SURFACE;

  /* Listen to the various events it can emit */
  view->map.notify = xdg_surface_map_notify;
  wl_signal_add(&xdg_surface->events.map, &view->map);

  view->unmap.notify = xdg_surface_unmap_notify;
  wl_signal_add(&xdg_surface->events.unmap, &view->unmap);

  view->destroy.notify = xdg_surface_destroy_notify;
  wl_signal_add(&xdg_surface->events.destroy, &view->destroy);

  /* cotd */
  struct wlr_xdg_toplevel *toplevel = xdg_surface->toplevel;

  view->request_move.notify = xdg_toplevel_request_move_notify;
  wl_signal_add(&toplevel->events.request_move, &view->request_move);

  view->request_resize.notify = xdg_toplevel_request_resize_notify;
  wl_signal_add(&toplevel->events.request_resize, &view->request_resize);

  view->request_maximize.notify = xdg_toplevel_request_maximize_notify;
  wl_signal_add(&toplevel->events.request_maximize, &view->request_maximize);

  /* Add it to the list of views. */
  wl_list_insert(&server->views, &view->link);
}

void wm_server::run() {
  wlr_log_init(WLR_DEBUG, NULL);

  wl_display = wl_display_create();

  backend = wlr_backend_autocreate(wl_display, NULL);

  renderer = wlr_backend_get_renderer(backend);
  wlr_renderer_init_wl_display(renderer, wl_display);


  wlr_compositor *compositor = wlr_compositor_create(wl_display, renderer);
  wlr_data_device_manager_create(wl_display);

  output_layout = wlr_output_layout_create();

  wl_list_init(&outputs);
  new_output.notify = new_output_notify;
  wl_signal_add(&backend->events.new_output, &new_output);

  wl_list_init(&views);
  xdg_shell = wlr_xdg_shell_create(wl_display);
  new_xdg_surface.notify = new_xdg_surface_notify;
  wl_signal_add(&xdg_shell->events.new_surface, &new_xdg_surface);

  cursor = wlr_cursor_create();
  wlr_cursor_attach_output_layout(cursor, output_layout);

  cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
  wlr_xcursor_manager_load(cursor_mgr, 1);
  wlr_xcursor_manager_load(cursor_mgr, 2);
  wlr_xcursor_manager_load(cursor_mgr, 3);

  cursor_motion.notify = cursor_motion_notify;
  wl_signal_add(&cursor->events.motion, &cursor_motion);

  cursor_motion_absolute.notify = cursor_motion_absolute_notify;
  wl_signal_add(&cursor->events.motion_absolute, &cursor_motion_absolute);

  cursor_button.notify = cursor_button_notify;
  wl_signal_add(&cursor->events.button, &cursor_button);

  cursor_axis.notify = cursor_axis_notify;
  wl_signal_add(&cursor->events.axis, &cursor_axis);

  cursor_frame.notify = cursor_frame_notify;
  wl_signal_add(&cursor->events.frame, &cursor_frame);

  wl_list_init(&keyboards);

  new_input.notify = new_input_notify;
  wl_signal_add(&backend->events.new_input, &new_input);

  seat = wlr_seat_create(wl_display, "seat0");

  request_cursor.notify = seat_request_cursor_notify;
  wl_signal_add(&seat->events.request_set_cursor, &request_cursor);

  const char *socket = wl_display_add_socket_auto(wl_display);
  if (!socket) {
    wlr_backend_destroy(backend);
    return;
  }

  if (!wlr_backend_start(backend)) {
    wlr_backend_destroy(backend);
    wl_display_destroy(wl_display);
    return;
  }

  setenv("WAYLAND_DISPLAY", socket, true);

  wlr_data_device_manager_create(wl_display);

  xwayland = wlr_xwayland_create(wl_display, compositor, true);
  wlr_xwayland_set_seat(xwayland, seat);

  new_xwayland_surface.notify = new_xwayland_surface_notify;
  wl_signal_add(&xwayland->events.new_surface, &new_xwayland_surface);
  wlr_log(WLR_INFO, "XWayland DISPLAY=%s", xwayland->display_name);

  wlr_log(WLR_INFO, "Wayland WAYLAND_DISPLAY=%s", socket);
  wl_display_run(wl_display);
}

void wm_server::destroy() {
  wlr_log(WLR_ERROR, "Quitting!");

  wlr_xcursor_manager_destroy(cursor_mgr);

  // wlr_xwayland_destroy(xwayland);
  // wlr_seat_destroy(seat);
  // wlr_xcursor_manager_destroy(cursor_mgr);
  wlr_cursor_destroy(cursor);
  wlr_output_layout_destroy(output_layout);

  wm_keyboard *keyboard, *tmp;
  wl_list_for_each_safe(keyboard, tmp, &keyboards, link) {
    delete keyboard;
  }

  /* Once wl_display_run returns, we shut down the  */
  wl_display_destroy_clients(wl_display);

  // wlr_renderer_destroy(renderer);
  wlr_backend_destroy(backend);

  wl_display_destroy(wl_display);
}

static bool handle_alt_keybinding(wm_server *server, xkb_keysym_t sym) {
  /*
   * Here we handle compositor keybindings. This is when the compositor is
   * processing keys, rather than passing them on to the client for its own
   * processing.
   *
   * This function assumes Alt is held down.
   */
  switch (sym) {
  case XKB_KEY_Escape:
    wl_display_terminate(server->wl_display);
    break;
  default:
    return false;
  }
  return true;
}

static bool handle_ctrl_keybinding(wm_server *server, xkb_keysym_t sym) {
  switch (sym) {

  case XKB_KEY_Return: {
    if (fork() == 0) {
      execl("/bin/sh", "/bin/sh", "-c", "tilix", (void *)NULL);
    }
    break;
  }

  default:
    return false;
  }

  return true;
}

static void keyboard_modifiers_notify(wl_listener *listener, void *data) {
  wm_keyboard *keyboard = wl_container_of(listener, keyboard, modifiers);
  wlr_seat_set_keyboard(keyboard->server->seat, keyboard->device);
  wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,
    &keyboard->device->keyboard->modifiers);
}

static void keyboard_key_notify(wl_listener *listener, void *data) {
  /* This event is raised when a key is pressed or released. */
  wm_keyboard *keyboard = wl_container_of(listener, keyboard, key);
  wm_server *server = keyboard->server;
  auto event = static_cast<struct wlr_event_keyboard_key *>(data);
  wlr_seat *seat = server->seat;

  uint32_t keycode = event->keycode + 8;
  const xkb_keysym_t *syms;
  int nsyms = xkb_state_key_get_syms(keyboard->device->keyboard->xkb_state, keycode, &syms);

  bool handled = false;
  uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->device->keyboard);
  if ((modifiers & WLR_MODIFIER_ALT) && event->state == WLR_KEY_PRESSED) {
    for (int i = 0; i < nsyms; i++) {
      handled = handle_alt_keybinding(server, syms[i]);
    }
  }

  if ((modifiers & WLR_MODIFIER_CTRL) && event->state == WLR_KEY_PRESSED) {
    for (int i = 0; i < nsyms; i++) {
      handled = handle_ctrl_keybinding(server, syms[i]);
    }
  }

  for (int i = 0; i < nsyms; i++) {
    switch (syms[i]) {
    case XKB_KEY_F12:
      wl_display_terminate(server->wl_display);
      handled = true;
      break;
    }
  }

  if (!handled) {
    wlr_seat_set_keyboard(seat, keyboard->device);
    wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode, event->state);
  }
}

void wm_server::new_keyboard(wlr_input_device *device) {
  wm_keyboard *keyboard = new wm_keyboard();
  keyboard->server = this;
  keyboard->device = device;

  /* We need to prepare an XKB keymap and assign it to the keyboard. This
   * assumes the defaults (e.g. layout = "us"). */
  xkb_rule_names rules;
  memset(&rules, 0, sizeof(xkb_rule_names));
  xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  xkb_keymap *keymap = xkb_map_new_from_names(context, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);

  wlr_keyboard_set_keymap(device->keyboard, keymap);
  xkb_keymap_unref(keymap);
  xkb_context_unref(context);
  wlr_keyboard_set_repeat_info(device->keyboard, 25, 600);

  /* Here we set up listeners for keyboard events. */
  keyboard->modifiers.notify = keyboard_modifiers_notify;
  wl_signal_add(&device->keyboard->events.modifiers, &keyboard->modifiers);

  keyboard->key.notify = keyboard_key_notify;
  wl_signal_add(&device->keyboard->events.key, &keyboard->key);

  wlr_seat_set_keyboard(seat, device);

  /* And add the keyboard to our list of keyboards */
  wl_list_insert(&keyboards, &keyboard->link);
}

void wm_server::new_pointer(wlr_input_device *device) {
  bool is_libinput = wlr_input_device_is_libinput(device);
  if (is_libinput) {
    libinput_device *libinput_device = wlr_libinput_get_device_handle(device);
    libinput_device_config_tap_set_enabled(libinput_device, LIBINPUT_CONFIG_TAP_ENABLED);
    libinput_device_config_scroll_set_natural_scroll_enabled(libinput_device, 1);
    libinput_device_config_accel_set_profile(libinput_device, LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT);
  }

  wlr_cursor_attach_input_device(cursor, device);
}


wm_view* wm_server::desktop_view_at(double lx, double ly,
  wlr_surface **surface, double *sx, double *sy) {

  wm_view *view;
  wl_list_for_each(view, &views, link) {

    double scaled_lx = 0;
    double scaled_ly = 0;
    view->scale_coords(lx, ly, &scaled_lx, &scaled_ly);
    if (view->view_at(scaled_lx, scaled_ly, surface, sx, sy)) {
      return view;
    }
  }
  return NULL;
}

void wm_server::process_cursor_move(uint32_t time) {
  grabbed_view->scale_coords(cursor->x, cursor->y, &grabbed_view->x, &grabbed_view->y);
  grabbed_view->x -= grab_x;
  grabbed_view->y -= grab_y;
}

void wm_server::process_cursor_resize(uint32_t time) {
  wm_view *view = grabbed_view;
  double dx = 0;
  double dy = 0;
  view->scale_coords(cursor->x, cursor->y, &dx, &dy);

  dx -= grab_x;
  dy -= grab_y;

  double x = view->x;
  double y = view->y;
  int width = grab_width;
  int height = grab_height;

  if (resize_edges & WLR_EDGE_TOP) {
    y = grab_y + dy;
    height -= dy;
    if (height < 1) {
      y += height;
    }
  } else if (resize_edges & WLR_EDGE_BOTTOM) {
    height += dy;
  }

  if (resize_edges & WLR_EDGE_LEFT) {
    x = grab_x + dx;
    width -= dx;
    if (width < 1) {
      x += width;
    }
  } else if (resize_edges & WLR_EDGE_RIGHT) {
    width += dx;
  }

  view->x = x;
  view->y = y;

  view->set_size(width, height);
}

void wm_server::process_cursor_motion(uint32_t time) {
  /* If the mode is non-passthrough, delegate to those functions. */
  if (cursor_mode == wm_CURSOR_MOVE) {
    process_cursor_move(time);
    return;
  } else if (cursor_mode == wm_CURSOR_RESIZE) {
    process_cursor_resize(time);
    return;
  }

  /* Otherwise, find the view under the pointer and send the event along. */
  double sx, sy;
  wlr_surface *surface = NULL;
  wm_view *view = desktop_view_at(cursor->x, cursor->y, &surface, &sx, &sy);

  if (!view) {
    /* If there's no view under the cursor, set the cursor image to a
     * default. This is what makes the cursor image appear when you move it
     * around the screen, not over any views. */
    wlr_xcursor_manager_set_cursor_image(cursor_mgr, "left_ptr", cursor);
  }

  if (surface) {
    bool focus_changed = seat->pointer_state.focused_surface != surface;
    /*
     * "Enter" the surface if necessary. This lets the client know that the
     * cursor has entered one of its surfaces.
     *
     * Note that this gives the surface "pointer focus", which is distinct
     * from keyboard focus. You get pointer focus by moving the pointer over
     * a window.
     */

    wlr_seat_pointer_notify_enter(seat, surface, sx, sy);

    if (!focus_changed) {
      /* The enter event contains coordinates, so we only need to notify
       * on motion if the focus did not change. */
      wlr_seat_pointer_notify_motion(seat, time, sx, sy);
    }
  } else {
    /* Clear pointer focus so future button events and such are not sent to
     * the last client to have the cursor over it. */
    wlr_seat_pointer_clear_focus(seat);
  }
}

wm_view* wm_server::view_from_surface(wlr_surface *surface) {
  wm_view *view;
  wl_list_for_each(view, &views, link) {
    if (view->surface() == surface) {
      return view;
    }
  }
  return NULL;
}

void wm_server::position_view(wm_view *view) {
  bool is_child = view->is_child();

  if (!is_child) {
    return;
  }

  wm_view *parent_view = view->parent();

  wlr_box parent_geometry;
  parent_view->geometry(&parent_geometry);

  wlr_box geometry;
  view->geometry(&geometry);

  int inside_x = (parent_geometry.width - geometry.width) / 2.0;
  int inside_y = (parent_geometry.height - geometry.height) / 2.0;

  view->x = parent_view->x + inside_x;
  view->y = parent_view->y + inside_y;
}

void wm_server::pop_view(wm_view *old_view) {
  wm_view *view;
  wl_list_for_each(view, &views, link) {
    if (view->mapped) {
      view->focus();
      return;
    }
  }
}
