#include "controller.h"

extern "C" {
  #include <unistd.h>
  #include <wayland-server-core.h>
  #include <wayland-contrib.h>
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
  #include <wlr/types/wlr_output_damage.h>
  #include <wlr/xwayland.h>
  #include <wlr/util/log.h>
  #include <wlr/backend/libinput.h>
  #include <xkbcommon/xkbcommon.h>
}

#include <memory>
#include <vector>

#include "keyboard.h"
#include "views/xwayland_view.h"
#include "views/xdg_view.h"
#include "output.h"

#include "key_bindings/key_binding_cmd.h"
#include "key_bindings/key_binding_quit.h"
#include "key_bindings/key_binding_dock_right.h"
#include "key_bindings/key_binding_dock_left.h"
#include "key_bindings/key_binding_maximize.h"

Controller::Controller()
  : CursorMode(WM_CURSOR_NONE) {
}

void Controller::quit() {
  wl_display_terminate(wl_display);
}

void Controller::remove_output(const Output *output) {
  wlr_output_layout_remove(output_layout, output->output());
}

void Controller::damage_outputs() {
  Output *output;
  wl_list_for_each(output, &outputs, link) {
    output->take_whole_damage();
  }
}

void Controller::damage_output(const View *view) {
  Output *output;
  wl_list_for_each(output, &outputs, link) {
    output->take_damage(view);
  }
}

bool Controller::handle_key(uint32_t keycode, const xkb_keysym_t *syms,
  int nsyms, uint32_t modifiers, int state) {
  bool handled = false;

  for (int i = 0; i < nsyms; i++) {
    for (auto &key_binding : key_bindings) {
      bool matched = key_binding->matches(modifiers, syms[i], (wlr_key_state)state);
      if (matched) {
        key_binding->run();
        handled = true;
      }
    }
  }

  return handled;
}

static void new_input_notify(wl_listener *listener, void *data) {
  Controller *server = wl_container_of(listener, server, new_input);
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

  uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
  if (!wl_list_empty(&server->keyboards)) {
    caps |= WL_SEAT_CAPABILITY_KEYBOARD;
  }

  wlr_seat_set_capabilities(server->seat, caps);
}

static void seat_request_cursor_notify(wl_listener *listener, void *data) {
  Controller *server = wl_container_of(listener, server, request_cursor);

  auto event = static_cast<wlr_seat_pointer_request_set_cursor_event*>(data);
  wlr_seat_client *focused_client = server->seat->pointer_state.focused_client;

  if (focused_client == event->seat_client) {
    wlr_cursor_set_surface(server->cursor, event->surface, event->hotspot_x, event->hotspot_y);
  }
}

static void cursor_motion_notify(wl_listener *listener, void *data) {
  Controller *server = wl_container_of(listener, server, cursor_motion);
  auto event = static_cast<struct wlr_event_pointer_motion*>(data);
  wlr_cursor_move(server->cursor, event->device, event->delta_x, event->delta_y);
  server->process_cursor_motion(event->time_msec);
}

static void cursor_motion_absolute_notify(wl_listener *listener, void *data) {
  Controller *server = wl_container_of(listener, server, cursor_motion_absolute);
  auto event = static_cast<struct wlr_event_pointer_motion_absolute*>(data);
  wlr_cursor_warp_absolute(server->cursor, event->device, event->x, event->y);

  server->process_cursor_motion(event->time_msec);
}

static void cursor_button_notify(wl_listener *listener, void *data) {
  Controller *server = wl_container_of(listener, server, cursor_button);
  auto event = static_cast<struct wlr_event_pointer_button*>(data);
  wlr_seat_pointer_notify_button(server->seat, event->time_msec, event->button, event->state);

  double sx, sy;
  wlr_surface *surface;
  View *view = server->desktop_view_at(server->cursor->x, server->cursor->y, &surface, &sx, &sy);

  if (event->state == WLR_BUTTON_RELEASED) {
    server->CursorMode = WM_CURSOR_PASSTHROUGH;
    return;
  }

  if (view != NULL) {
    view->focus_view(surface);
  }
}

static void cursor_axis_notify(wl_listener *listener, void *data) {
  Controller *server = wl_container_of(listener, server, cursor_axis);
  auto event = static_cast<wlr_event_pointer_axis*>(data);
  /* Notify the client with pointer focus of the axis event. */
  wlr_seat_pointer_notify_axis(server->seat, event->time_msec, event->orientation,
    event->delta, event->delta_discrete, event->source);
}

static void cursor_frame_notify(wl_listener *listener, void *data) {
  Controller *server = wl_container_of(listener, server, cursor_frame);
  wlr_seat_pointer_notify_frame(server->seat);
}

static void output_frame_notify(wl_listener *listener, void *data) {
  Output *output = wl_container_of(listener, output, frame_);
  output->render();
}

static void output_destroy_notify(wl_listener *listener, void *data) {
  Output *output = wl_container_of(listener, output, destroy_);
  output->destroy();
  wl_list_remove(&output->link);
  // delete output;
}

static void new_output_notify(wl_listener *listener, void *data) {
  Controller *server = wl_container_of(listener, server, new_output);
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

  if (strcmp(wlr_output->name, "DP-0") == 0 ||
      strcmp(wlr_output->name, "DP-1") == 0 ||
      strcmp(wlr_output->name, "DP-2") == 0) {
    wlr_output_set_scale(wlr_output, 2);
  }

  auto damage = wlr_output_damage_create(wlr_output);

  Output *output = new Output(server, wlr_output, damage);

  output->frame_.notify = output_frame_notify;
  wl_signal_add(&damage->events.frame, &output->frame_);

  output->destroy_.notify = output_destroy_notify;
  wl_signal_add(&wlr_output->events.destroy, &output->destroy_);

  wlr_output_layout_add_auto(server->output_layout, wlr_output);

  wlr_output_create_global(wlr_output);

  wl_list_insert(&server->outputs, &output->link);
}

static void xdg_surface_map_notify(wl_listener *listener, void *data) {
  View *view = wl_container_of(listener, view, map);
  view->map_view();
}

static void xdg_surface_unmap_notify(wl_listener *listener, void *data) {
  View *view = wl_container_of(listener, view, unmap);
  view->unmap_view();
}

static void xdg_surface_destroy_notify(wl_listener *listener, void *data) {
  View *view = wl_container_of(listener, view, destroy);
  wl_list_remove(&view->link);
  // delete view;
}

static void xdg_popup_subsurface_commit_notify(wl_listener *listener, void *data) {
  Subsurface *subsurface = wl_container_of(listener, subsurface, commit);
  subsurface->server->damage_outputs();
}

static void xdg_subsurface_commit_notify(wl_listener *listener, void *data) {
  Subsurface *subsurface = wl_container_of(listener, subsurface, commit);
  subsurface->server->damage_outputs();
}

static void xdg_popup_destroy_notify(wl_listener *listener, void *data) {
  Popup *popup = wl_container_of(listener, popup, destroy);
  popup->server->damage_outputs();
}

static void xdg_popup_commit_notify(wl_listener *listener, void *data) {
  Popup *popup = wl_container_of(listener, popup, commit);
  popup->server->damage_outputs();
}

static void xdg_surface_commit_notify(wl_listener *listener, void *data) {
  View *view = wl_container_of(listener, view, commit);
  if (view->mapped) {
    view->committed();
  }
}

static void xdg_toplevel_request_move_notify(wl_listener *listener, void *data) {
  View *view = wl_container_of(listener, view, request_move);
  view->window(false);
  view->begin_interactive(WM_CURSOR_MOVE, 0);
}

static void xdg_toplevel_request_resize_notify(wl_listener *listener, void *data) {
  auto event = static_cast<wlr_xdg_toplevel_resize_event*>(data);
  View *view = wl_container_of(listener, view, request_resize);
  view->begin_interactive(WM_CURSOR_RESIZE, event->edges);
}

static void xdg_toplevel_request_maximize_notify(wl_listener *listener, void *data) {
  View *view = wl_container_of(listener, view, request_maximize);
  view->toggle_maximized();
}

static void xwayland_surface_request_configure_notify(wl_listener *listener, void *data) {
  XWaylandView *view = wl_container_of(listener, view, request_configure);
  auto event = static_cast<wlr_xwayland_surface_configure_event*>(data);
  view->x = event->x;
  view->y = event->y;
  wlr_xwayland_surface_configure(event->surface, event->x, event->y, event->width, event->height);
}

static void new_xwayland_surface_notify(wl_listener *listener, void *data) {
  Controller *server = wl_container_of(listener, server, new_xwayland_surface);
  auto xwayland_surface = static_cast<wlr_xwayland_surface*>(data);

  auto view = new XWaylandView(server, xwayland_surface);

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

  wl_list_insert(&server->views, &view->link);
}

static void new_popup_subsurface_notify(wl_listener *listener, void *data) {
  auto *wlr_subsurface_ = static_cast<wlr_subsurface*>(data);
  Popup *popup = wl_container_of(listener, popup, new_subsurface);

  auto subsurface = new Subsurface();
  subsurface->server = popup->server;

  subsurface->commit.notify = xdg_popup_subsurface_commit_notify;
  wl_signal_add(&wlr_subsurface_->surface->events.commit, &subsurface->commit);
}

static void new_subsurface_notify(wl_listener *listener, void *data) {
  auto *wlr_subsurface_ = static_cast<wlr_subsurface*>(data);
  View *view = wl_container_of(listener, view, new_subsurface);

  auto subsurface = new Subsurface();
  subsurface->server = view->server;

  subsurface->commit.notify = xdg_subsurface_commit_notify;
  wl_signal_add(&wlr_subsurface_->surface->events.commit, &subsurface->commit);
}

static void new_popup_popup_notify(wl_listener *listener, void *data) {
  auto *xdg_popup = static_cast<wlr_xdg_popup*>(data);
  Popup *parent_popup = wl_container_of(listener, parent_popup, new_popup);

  auto popup = new Popup();
  popup->server = parent_popup->server;

  popup->commit.notify = xdg_popup_commit_notify;
  wl_signal_add(&xdg_popup->base->surface->events.commit, &popup->commit);

  popup->destroy.notify = xdg_popup_destroy_notify;
  wl_signal_add(&xdg_popup->base->surface->events.destroy, &popup->destroy);

  popup->new_subsurface.notify = new_popup_subsurface_notify;
  wl_signal_add(&xdg_popup->base->surface->events.new_subsurface, &popup->new_subsurface);

  popup->new_popup.notify = new_popup_popup_notify;
  wl_signal_add(&xdg_popup->base->events.new_popup, &popup->new_popup);
}

static void new_popup_notify(wl_listener *listener, void *data) {
  auto *xdg_popup = static_cast<wlr_xdg_popup*>(data);

  View *view = wl_container_of(listener, view, new_popup);

  auto popup = new Popup();
  popup->server = view->server;

  popup->commit.notify = xdg_popup_commit_notify;
  wl_signal_add(&xdg_popup->base->surface->events.commit, &popup->commit);

  popup->destroy.notify = xdg_popup_destroy_notify;
  wl_signal_add(&xdg_popup->base->surface->events.destroy, &popup->destroy);

  popup->new_subsurface.notify = new_popup_subsurface_notify;
  wl_signal_add(&xdg_popup->base->surface->events.new_subsurface, &popup->new_subsurface);

  popup->new_popup.notify = new_popup_popup_notify;
  wl_signal_add(&xdg_popup->base->events.new_popup, &popup->new_popup);
}

static void new_xdg_surface_notify(wl_listener *listener, void *data) {
  /* This event is raised when wlr_xdg_shell receives a new xdg surface from a
   * client, either a toplevel (application window) or popup. */
  Controller *server = wl_container_of(listener, server, new_xdg_surface);
  auto xdg_surface = static_cast<wlr_xdg_surface*>(data);
  if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
    return;
  }

  /* Allocate a wm_view for this surface */
  auto view = new XdgView(server, xdg_surface);

  /* Listen to the various events it can emit */
  view->map.notify = xdg_surface_map_notify;
  wl_signal_add(&xdg_surface->events.map, &view->map);

  view->unmap.notify = xdg_surface_unmap_notify;
  wl_signal_add(&xdg_surface->events.unmap, &view->unmap);

  view->destroy.notify = xdg_surface_destroy_notify;
  wl_signal_add(&xdg_surface->events.destroy, &view->destroy);

  view->commit.notify = xdg_surface_commit_notify;
  wl_signal_add(&xdg_surface->surface->events.commit, &view->commit);

  view->new_subsurface.notify = new_subsurface_notify;
  wl_signal_add(&xdg_surface->surface->events.new_subsurface, &view->new_subsurface);

  view->new_popup.notify = new_popup_notify;
  wl_signal_add(&xdg_surface->events.new_popup, &view->new_popup);

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

void Controller::toggle_maximize() {
  int view_count = wl_list_length(&views);
  if (view_count <= 0)
    return;

  View *view = wl_list_first(view, &views, link);
  view->toggle_maximized();
}

void Controller::dock_left() {
  int view_count = wl_list_length(&views);
  if (view_count <= 0)
    return;

  View *view = wl_list_first(view, &views, link);
  view->tile_left();
  damage_outputs();
}

void Controller::dock_right() {
  int view_count = wl_list_length(&views);
  if (view_count <= 0)
    return;

  View *view = wl_list_first(view, &views, link);
  view->tile_right();
  damage_outputs();
}

void Controller::init_keybindings() {
  auto terminal = std::make_shared<key_binding_cmd>();
  terminal->ctrl = true;
  terminal->key = XKB_KEY_Return;
  terminal->cmd = "tilix";
  terminal->state = WLR_KEY_PRESSED;
  key_bindings.push_back(terminal);

  auto quit_2 = std::make_shared<key_binding_quit>(this);
  quit_2->ctrl = true;
  quit_2->key = XKB_KEY_q;
  quit_2->state = WLR_KEY_PRESSED;
  key_bindings.push_back(quit_2);

  auto quit = std::make_shared<key_binding_quit>(this);
  quit->alt = true;
  quit->ctrl = true;
  quit->key = XKB_KEY_BackSpace;
  quit->state = WLR_KEY_PRESSED;
  key_bindings.push_back(quit);

  auto dock_left = std::make_shared<key_binding_dock_left>(this);
  dock_left->super = true;
  dock_left->key = XKB_KEY_Left;
  dock_left->state = WLR_KEY_PRESSED;
  key_bindings.push_back(dock_left);

  auto dock_right = std::make_shared<key_binding_dock_right>(this);
  dock_right->super = true;
  dock_right->key = XKB_KEY_Right;
  dock_right->state = WLR_KEY_PRESSED;
  key_bindings.push_back(dock_right);

  auto maximize = std::make_shared<key_binding_maximize>(this);
  maximize->super = true;
  maximize->key = XKB_KEY_Up;
  maximize->state = WLR_KEY_PRESSED;
  key_bindings.push_back(maximize);
}

void Controller::run() {
  wlr_log_init(WLR_DEBUG, NULL);

  init_keybindings();

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

void Controller::destroy() {
  wlr_log(WLR_ERROR, "Quitting!");

  wlr_xcursor_manager_destroy(cursor_mgr);

  wlr_cursor_destroy(cursor);

  Keyboard *keyboard, *tmp;
  wl_list_for_each_safe(keyboard, tmp, &keyboards, link) {
    delete keyboard;
  }

  wl_display_destroy_clients(wl_display);

  wlr_backend_destroy(backend);
  wlr_output_layout_destroy(output_layout);
  wl_display_destroy(wl_display);
}

static void keyboard_modifiers_notify(wl_listener *listener, void *data) {
  Keyboard *keyboard = wl_container_of(listener, keyboard, modifiers);
  wlr_seat_set_keyboard(keyboard->server->seat, keyboard->device);
  wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,
    &keyboard->device->keyboard->modifiers);
}

static void keyboard_key_notify(wl_listener *listener, void *data) {
  /* This event is raised when a key is pressed or released. */
  Keyboard *keyboard = wl_container_of(listener, keyboard, key);
  Controller *server = keyboard->server;
  auto event = static_cast<struct wlr_event_keyboard_key *>(data);
  wlr_seat *seat = server->seat;

  uint32_t keycode = event->keycode + 8;
  const xkb_keysym_t *syms;
  int nsyms = xkb_state_key_get_syms(keyboard->device->keyboard->xkb_state, keycode, &syms);
  uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->device->keyboard);

  bool handled = server->handle_key(keycode, syms, nsyms, modifiers, event->state);

  if (!handled) {
    wlr_seat_set_keyboard(seat, keyboard->device);
    wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode, event->state);
  }
}

void Controller::new_keyboard(wlr_input_device *device) {
  auto keyboard = new Keyboard();
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
  wlr_keyboard_set_repeat_info(device->keyboard, 25, 200);

  /* Here we set up listeners for keyboard events. */
  keyboard->modifiers.notify = keyboard_modifiers_notify;
  wl_signal_add(&device->keyboard->events.modifiers, &keyboard->modifiers);

  keyboard->key.notify = keyboard_key_notify;
  wl_signal_add(&device->keyboard->events.key, &keyboard->key);

  wlr_seat_set_keyboard(seat, device);

  /* And add the keyboard to our list of keyboards */
  wl_list_insert(&keyboards, &keyboard->link);
}

void Controller::new_pointer(wlr_input_device *device) {
  bool is_libinput = wlr_input_device_is_libinput(device);
  if (is_libinput) {
    libinput_device *libinput_device = wlr_libinput_get_device_handle(device);
    libinput_device_config_tap_set_enabled(libinput_device, LIBINPUT_CONFIG_TAP_ENABLED);
    libinput_device_config_scroll_set_natural_scroll_enabled(libinput_device, 1);
    libinput_device_config_accel_set_profile(libinput_device, LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT);
    libinput_device_config_accel_set_speed(libinput_device, -0.5);
  }

  wlr_cursor_attach_input_device(cursor, device);
}

View* Controller::desktop_view_at(double lx, double ly,
  wlr_surface **surface, double *sx, double *sy) {
  View *view;
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

void Controller::process_cursor_move(uint32_t time) {
  grabbed_view->scale_coords(cursor->x, cursor->y, &grabbed_view->x, &grabbed_view->y);
  grabbed_view->x -= grab_x;
  grabbed_view->y -= grab_y;
}

void Controller::process_cursor_resize(uint32_t time) {
  View *view = grabbed_view;

  double dx = 0;
  double dy = 0;

  view->scale_coords(cursor->x, cursor->y, &dx, &dy);

  dx -= grab_cursor_x;
  dy -= grab_cursor_y;

  double x = view->x;
  double y = view->y;

  int width = grab_width;
  int height = grab_height;

  if (resize_edges & WLR_EDGE_TOP) {
    y = grab_y + dy;
    height = grab_height - dy;
  } else if (resize_edges & WLR_EDGE_BOTTOM) {
    height += dy;
  }

  if (resize_edges & WLR_EDGE_LEFT) {
    x = grab_x + dx;
    width -= dx;
  } else if (resize_edges & WLR_EDGE_RIGHT) {
    width += dx;
  }

  view->x = x;
  view->y = y;

  view->resize(width, height);
}

void Controller::process_cursor_motion(uint32_t time) {
  /* If the mode is non-passthrough, delegate to those functions. */
  if (CursorMode == WM_CURSOR_MOVE) {
    process_cursor_move(time);
    damage_outputs();
    return;
  } else if (CursorMode == WM_CURSOR_RESIZE) {
    process_cursor_resize(time);
    return;
  }

  /* Otherwise, find the view under the pointer and send the event along. */
  double sx, sy;
  wlr_surface *surface = NULL;
  View *view = desktop_view_at(cursor->x, cursor->y, &surface, &sx, &sy);

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

View* Controller::view_from_surface(wlr_surface *surface) {
  View *view;
  wl_list_for_each(view, &views, link) {
    if (view->surface() == surface) {
      return view;
    }
  }
  return NULL;
}

void Controller::position_view(View *view) {
  bool is_child = view->is_child();

  wlr_box geometry;
  view->geometry(&geometry);

  if (!is_child) {
    wlr_output* output = wlr_output_layout_output_at(output_layout, cursor->x, cursor->y);
    int inside_x = ((output->width  / output->scale) - geometry.width) / 2.0;
    int inside_y = ((output->height / output->scale) - geometry.height) / 2.0;

    view->x = inside_x;
    view->y = inside_y;
    return;
  }

  View *parent_view = view->parent();

  wlr_box parent_geometry;
  parent_view->geometry(&parent_geometry);

  int inside_x = (parent_geometry.width - geometry.width) / 2.0;
  int inside_y = (parent_geometry.height - geometry.height) / 2.0;

  view->x = parent_view->x + inside_x;
  view->y = parent_view->y + inside_y;
}

void Controller::focus_top() {
  View *view;
  wl_list_for_each(view, &views, link) {
    if (view->mapped) {
      view->focus();
      return;
    }
  }
}
