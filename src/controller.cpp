#include "controller.h"

#include <iostream>
#include <memory>
#include <vector>

#include "wlroots.h"

#include "keyboard.h"
#include "view.h"
#include "output.h"

#include "key_bindings/key_binding_cmd.h"
#include "key_bindings/key_binding_quit.h"
#include "key_bindings/key_binding_dock_right.h"
#include "key_bindings/key_binding_dock_left.h"
#include "key_bindings/key_binding_maximize.h"

Server::Server()
  : CursorMode(WM_CURSOR_NONE) {
}

void Server::quit() {
  wl_display_terminate(wl_display);
}

void Server::remove_output(const Output *output) {
  wlr_output_layout_remove(output_layout, output->output());

  auto condition = [output](auto &el) { return el.get() == output; };
  auto result = std::find_if(outputs_.begin(), outputs_.end(), condition);
  if (result != outputs_.end()) {
    outputs_.erase(result);
  }
}

void Server::damage_outputs() {
  for (auto &output : outputs_) {
    output->take_whole_damage();
  }
}

bool layout_intersects(wlr_output_layout *layout, const Output *output, const View *view) {
  wlr_box geometry;
  view->extents(&geometry);
  geometry.x += view->x;
  geometry.y += view->y;
  bool intersects = wlr_output_layout_intersects(layout, output->output_, &geometry);
  return intersects;
}

bool Server::handle_key(uint32_t keycode, const xkb_keysym_t *syms,
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
  Server *server = wl_container_of(listener, server, new_input);
  auto device = static_cast<struct wlr_input_device *>(data);

  std::clog << device->name << std::endl;

  switch (device->type) {
  case WLR_INPUT_DEVICE_KEYBOARD:
    server->new_keyboard(device);
    break;

  case WLR_INPUT_DEVICE_POINTER:
    server->new_pointer(device);
    break;

  case WLR_INPUT_DEVICE_SWITCH:
    server->new_switch(device);
    break;

  default:
    break;
  }

  uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
  if (!server->keyboards_.empty()) {
    caps |= WL_SEAT_CAPABILITY_KEYBOARD;
  }

  wlr_seat_set_capabilities(server->seat, caps);
}

static void seat_request_cursor_notify(wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, request_cursor);

  auto event = static_cast<wlr_seat_pointer_request_set_cursor_event*>(data);
  wlr_seat_client *focused_client = server->seat->pointer_state.focused_client;

  if (focused_client == event->seat_client) {
    wlr_cursor_set_surface(server->cursor, event->surface, event->hotspot_x, event->hotspot_y);
  }
}

static void cursor_motion_notify(wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, cursor_motion);
  auto event = static_cast<struct wlr_event_pointer_motion*>(data);
  wlr_cursor_move(server->cursor, event->device, event->delta_x, event->delta_y);
  server->process_cursor_motion(event->time_msec);
}

static void cursor_motion_absolute_notify(wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, cursor_motion_absolute);
  auto event = static_cast<struct wlr_event_pointer_motion_absolute*>(data);
  wlr_cursor_warp_absolute(server->cursor, event->device, event->x, event->y);

  server->process_cursor_motion(event->time_msec);
}

static void cursor_button_notify(wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, cursor_button);
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
  Server *server = wl_container_of(listener, server, cursor_axis);
  auto event = static_cast<wlr_event_pointer_axis*>(data);
  /* Notify the client with pointer focus of the axis event. */
  wlr_seat_pointer_notify_axis(server->seat, event->time_msec, event->orientation,
    event->delta, event->delta_discrete, event->source);
}

static void cursor_frame_notify(wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, cursor_frame);
  wlr_seat_pointer_notify_frame(server->seat);
}

static void output_frame_notify(wl_listener *listener, void *data) {
  Output *output = wl_container_of(listener, output, frame_);
  if (output->output_->enabled) {
    output->render();
  }
}

static void output_destroy_notify(wl_listener *listener, void *data) {
  Output *output = wl_container_of(listener, output, destroy_);
  output->destroy();
}

void Server::add_output(std::shared_ptr<Output>& output) {
  outputs_.push_back(output);
}

static void new_output_notify(wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, new_output);
  auto wlr_output = static_cast<struct wlr_output*>(data);

  // wlr_log(WLR_ERROR, "%s connected", wlr_output->name);

  std::clog << wlr_output->name << " connected" << std::endl;

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
      std::cerr << "Failed to commit output" << std::endl;

      // wlr_log(WLR_ERROR, "Failed to commit output");
      return;
    }
  }

  int scale = 1;

  if (strcmp(wlr_output->name, "X11-1") == 0) {
    scale = 2;
  }

  if (strcmp(wlr_output->name, "eDP-1") == 0) {
    scale = 3;
  }

  if (strcmp(wlr_output->name, "DP-0") == 0 ||
      strcmp(wlr_output->name, "DP-1") == 0 ||
      strcmp(wlr_output->name, "DP-2") == 0) {
    scale = 2;
  }

  wlr_output_set_scale(wlr_output, scale);

  auto damage = wlr_output_damage_create(wlr_output);

  auto output = std::make_shared<Output>(server, wlr_output, damage, server->output_layout);

  output->frame_.notify = output_frame_notify;
  wl_signal_add(&damage->events.frame, &output->frame_);

  output->destroy_.notify = output_destroy_notify;
  wl_signal_add(&wlr_output->events.destroy, &output->destroy_);

  wlr_output_layout_add_auto(server->output_layout, wlr_output);

  wlr_output_create_global(wlr_output);

  server->add_output(output);

  // wl_list_insert(&server->outputs, &output->link);

  wlr_xcursor_manager_load(server->cursor_mgr, scale);
  wlr_xcursor_manager_set_cursor_image(server->cursor_mgr, "left_ptr", server->cursor);
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
  Server *server = view->server;

  auto condition = [view](auto &el) { return el.get() == view; };
  auto result = std::find_if(server->views_.begin(), server->views_.end(), condition);
  if (result != server->views_.end()) {
    server->views_.erase(result);
  }
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
  auto xdg_surface = static_cast<wlr_xdg_surface*>(data);
  if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
    return;
  }

  Server *server = wl_container_of(listener, server, new_xdg_surface);

  auto view = std::make_shared<View>(server, xdg_surface);

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

  server->views_.push_back(view);
}

void Server::toggle_maximize() {
  if (views_.empty()) return;

  auto &view = views_.front();
  view->toggle_maximized();
}

void Server::dock_left() {
  if (views_.empty()) return;

  auto &view = views_.front();
  view->tile_left();
  damage_outputs();
}

void Server::dock_right() {
  if (views_.empty()) return;

  auto &view = views_.front();
  view->tile_right();
  damage_outputs();
}

void Server::init_keybindings() {
  auto terminal = std::make_shared<key_binding_cmd>();
  terminal->ctrl = true;
  terminal->key = XKB_KEY_Return;
  terminal->cmd = "tilix";
  terminal->state = WLR_KEY_PRESSED;
  key_bindings.push_back(terminal);

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

static void handle_request_set_primary_selection(struct wl_listener *listener, void *data) {
  Server *controller = wl_container_of(listener, controller, request_set_primary_selection);
  auto event = static_cast<wlr_seat_request_set_primary_selection_event*>(data);
  wlr_seat_set_primary_selection(controller->seat, event->source, event->serial);
}

static void handle_request_set_selection(struct wl_listener *listener, void *data) {
  Server *controller = wl_container_of(listener, controller, request_set_selection);
  auto event = static_cast<wlr_seat_request_set_selection_event*>(data);
  wlr_seat_set_selection(controller->seat, event->source, event->serial);
}

void Server::run() {
  // wlr_log_init(WLR_DEBUG, NULL);

  init_keybindings();

  wl_display = wl_display_create();

  backend = wlr_backend_autocreate(wl_display, NULL);

  renderer = wlr_backend_get_renderer(backend);
  wlr_renderer_init_wl_display(renderer, wl_display);

  wlr_compositor_create(wl_display, renderer);
  wlr_data_device_manager_create(wl_display);

  output_layout = wlr_output_layout_create();

  new_output.notify = new_output_notify;
  wl_signal_add(&backend->events.new_output, &new_output);

  xdg_shell = wlr_xdg_shell_create(wl_display);
  new_xdg_surface.notify = new_xdg_surface_notify;
  wl_signal_add(&xdg_shell->events.new_surface, &new_xdg_surface);

  cursor = wlr_cursor_create();
  wlr_cursor_attach_output_layout(cursor, output_layout);

  cursor_mgr = wlr_xcursor_manager_create(NULL, 24);

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

  new_input.notify = new_input_notify;
  wl_signal_add(&backend->events.new_input, &new_input);

  seat = wlr_seat_create(wl_display, "seat0");

  request_cursor.notify = seat_request_cursor_notify;
  wl_signal_add(&seat->events.request_set_cursor, &request_cursor);

  wlr_data_control_manager_v1_create(wl_display);
  wlr_primary_selection_v1_device_manager_create(wl_display);

  request_set_selection.notify = handle_request_set_selection;
  wl_signal_add(&seat->events.request_set_selection, &request_set_selection);

  request_set_primary_selection.notify = handle_request_set_primary_selection;
  wl_signal_add(&seat->events.request_set_primary_selection, &request_set_primary_selection);

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

  std::clog << "Wayland WAYLAND_DISPLAY=" << socket << std::endl;

  // wlr_log(WLR_INFO, "Wayland WAYLAND_DISPLAY=%s", socket);
  wl_display_run(wl_display);
}

void Server::destroy() {
  std::clog << "Quitting!" << std::endl;
  // wlr_log(WLR_ERROR, "Quitting!");

  wlr_xcursor_manager_destroy(cursor_mgr);

  wlr_cursor_destroy(cursor);

  wl_display_destroy_clients(wl_display);

  wlr_backend_destroy(backend);
  wlr_output_layout_destroy(output_layout);
  wl_display_destroy(wl_display);
}

static void lid_notify(wl_listener *listener, void *data) {
  auto event = static_cast<wlr_event_switch_toggle *>(data);
  Server *server = wl_container_of(listener, server, lid);

  std::clog << "lid_notify" << std::endl;

  if (event->switch_type != WLR_SWITCH_TYPE_LID) {
    return;
  }

  const std::string laptop_screen_name = "eDP-1";

  if (event->switch_state == WLR_SWITCH_STATE_ON) {
    server->disconnect_output(laptop_screen_name, false);
  }

  if (event->switch_state == WLR_SWITCH_STATE_OFF) {
    server->disconnect_output(laptop_screen_name, true);
  }
}

void Server::disconnect_output(const std::string& name, bool enabled) {
  auto lambda = [name](auto &output) -> bool { return (name.compare(output->output_->name) == 0); };
  auto it = std::find_if(outputs_.begin(), outputs_.end(), lambda);
  if (it == outputs_.end()) {
    return;
  }

  auto &output = (*it);

  if (enabled) {
    wlr_output_enable(output->output_, true);
    wlr_output_layout_add_auto(output_layout, output->output_);
    wlr_output_set_scale(output->output_, 3);
  } else {
    wlr_output_enable(output->output_, false);
    wlr_output_layout_remove(output_layout, output->output_);
  }
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
  Server *server = keyboard->server;
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

void Server::new_keyboard(wlr_input_device *device) {
  auto keyboard = std::make_shared<Keyboard>();
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

  keyboards_.push_back(keyboard);
}

void Server::new_pointer(wlr_input_device *device) {
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

void Server::new_switch(wlr_input_device *device) {
  std::clog << "new_switch" << std::endl;
  lid.notify = lid_notify;
  wl_signal_add(&device->switch_device->events.toggle, &lid);
}

View* Server::desktop_view_at(double lx, double ly,
  wlr_surface **surface, double *sx, double *sy) {
  for (auto &view : views_) {
    if (view->view_at(lx, ly, surface, sx, sy)) {
      return view.get();
    }
  }
  return NULL;
}

void Server::process_cursor_move(uint32_t time) {
  grabbed_view->x = cursor->x - grab_x;
  grabbed_view->y = cursor->y - grab_y;
}

void Server::process_cursor_resize(uint32_t time) {
  View *view = grabbed_view;

  double dx = cursor->x - grab_cursor_x;
  double dy = cursor->y - grab_cursor_y;

  double x = view->x;
  double y = view->y;

  double width = grab_width;
  double height = grab_height;

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

  uint min_width = view->min_width();
  if (width > min_width) {
    view->x = x;
  }

  uint min_height = view->min_height();
  if (height > min_height) {
    view->y = y;
  }

  view->resize(width, height);
}

void Server::process_cursor_motion(uint32_t time) {
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

View* Server::view_from_surface(wlr_surface *surface) {
  for (auto &view : views_) {
    if (view->surface() == surface) {
      return view.get();
    }
  }
  return NULL;
}

void Server::position_view(View *view) {
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

void Server::focus_top() {
  for (auto &view : views_) {
    if (view->mapped) {
      view->focus();
      return;
    }
  }
}
