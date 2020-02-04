#include "server.h"

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
  : grab_state_({
    .view = NULL,
    .cursor_x = 0,
    .cursor_y = 0,
    .x = 0,
    .y = 0,
    .width = 0,
    .height = 0,
    .resize_edges = 0,
    .CursorMode = WM_CURSOR_NONE
    })
  { }

void Server::quit() {
  wl_display_terminate(display_);
}

void Server::remove_output(const Output *output) {
  wlr_output_layout_remove(layout_, output->output());

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
}

void Server::on_set_primary_selection(wlr_seat_request_set_primary_selection_event *event) {
  wlr_seat_set_primary_selection(seat_, event->source, event->serial);
}

void Server::on_set_selection(wlr_seat_request_set_selection_event *event) {
  wlr_seat_set_selection(seat_, event->source, event->serial);
}

void Server::on_set_cursor(wlr_seat_pointer_request_set_cursor_event *event) {
  wlr_seat_client *focused_client = seat_->pointer_state.focused_client;

  if (focused_client == event->seat_client) {
    wlr_cursor_set_surface(cursor_, event->surface, event->hotspot_x, event->hotspot_y);
  }
}

void Server::on_axis(wlr_event_pointer_axis *event) {
  wlr_seat_pointer_notify_axis(seat_, event->time_msec, event->orientation,
    event->delta, event->delta_discrete, event->source);
}

void Server::on_cursor_frame() {
  wlr_seat_pointer_notify_frame(seat_);
}

static void seat_request_cursor_notify(wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, request_cursor);
  auto event = static_cast<wlr_seat_pointer_request_set_cursor_event*>(data);
  server->on_set_cursor(event);
}

void Server::cursor_motion_notify(wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, cursor_motion);
  auto event = static_cast<struct wlr_event_pointer_motion*>(data);
  wlr_cursor_move(server->cursor_, event->device, event->delta_x, event->delta_y);
  server->process_cursor_motion(event->time_msec);
}

void Server::cursor_motion_absolute_notify(wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, cursor_motion_absolute);
  auto event = static_cast<struct wlr_event_pointer_motion_absolute*>(data);
  wlr_cursor_warp_absolute(server->cursor_, event->device, event->x, event->y);
  server->process_cursor_motion(event->time_msec);
}

static void cursor_button_notify(wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, cursor_button);
  auto event = static_cast<struct wlr_event_pointer_button*>(data);
  server->on_button(event);
}

void Server::focus_view(View *view) {
  wlr_surface *prev_surface = seat_->keyboard_state.focused_surface;
  const wlr_surface *surface = view->surface();

  if (prev_surface == surface) {
    return;
  }

  if (prev_surface) {
    View *previous_view = view_from_surface(seat_->keyboard_state.focused_surface);
    previous_view->unfocus();
  }

  auto condition = [view](auto &el) { return el.get() == view; };
  auto result = std::find_if(views_.begin(), views_.end(), condition);
  if (result != views_.end()) {
    auto resultValue = *result;
    views_.erase(result);
    views_.insert(views_.begin(), resultValue);
  }

  view->focus();
}

void Server::render_output(const Output *output) const {
  output->render(views_);
}

static void cursor_axis_notify(wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, cursor_axis);
  auto event = static_cast<wlr_event_pointer_axis*>(data);
  server->on_axis(event);
}

static void cursor_frame_notify(wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, cursor_frame);
  server->on_cursor_frame();
}

static void output_frame_notify(wl_listener *listener, void *data) {
  Output *output = wl_container_of(listener, output, frame_);
  output->frame();
}

static void output_destroy_notify(wl_listener *listener, void *data) {
  Output *output = wl_container_of(listener, output, destroy_);
  output->destroy();
}

void Server::add_output(const std::shared_ptr<Output>& output) {
  outputs_.push_back(output);
}

void Server::begin_interactive(View *view, CursorMode mode, unsigned int edges) {
  wlr_surface *focused_surface = seat_->pointer_state.focused_surface;

  if (view->surface() != focused_surface) {
    return;
  }

  grab_state_.x = 0;
  grab_state_.y = 0;

  grab_state_.view = view;
  grab_state_.CursorMode = mode;

  wlr_box geo_box;
  view->extents(&geo_box);

  if (mode == WM_CURSOR_MOVE) {
    grab_state_.x = cursor_->x - view->x;
    grab_state_.y = cursor_->y - view->y;
  } else {
    grab_state_.x = view->x;
    grab_state_.y = view->y;
    grab_state_.cursor_x = cursor_->x;
    grab_state_.cursor_y = cursor_->y;
  }

  grab_state_.width = geo_box.width;
  grab_state_.height = geo_box.height;

  grab_state_.resize_edges = edges;
}

void Server::on_new_output(wlr_output* wlr_output) {
  if (!wl_list_empty(&wlr_output->modes)) {
    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
    wlr_output_set_mode(wlr_output, mode);
    wlr_output_enable(wlr_output, true);
    if (!wlr_output_commit(wlr_output)) {
      std::cerr << "Failed to commit output" << std::endl;
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
  auto output = std::make_shared<Output>(this, wlr_output, renderer_, damage, layout_);

  output->frame_.notify = output_frame_notify;
  wl_signal_add(&damage->events.frame, &output->frame_);

  output->destroy_.notify = output_destroy_notify;
  wl_signal_add(&wlr_output->events.destroy, &output->destroy_);

  wlr_output_layout_add_auto(layout_, wlr_output);

  wlr_output_create_global(wlr_output);

  add_output(output);

  wlr_xcursor_manager_load(cursor_manager_, scale);
  wlr_xcursor_manager_set_cursor_image(cursor_manager_, "left_ptr", cursor_);
}

static void new_output_notify(wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, new_output);
  auto wlr_output = static_cast<struct wlr_output*>(data);
  server->on_new_output(wlr_output);
}

void Server::on_button(wlr_event_pointer_button *event) {
  wlr_seat_pointer_notify_button(seat_, event->time_msec, event->button, event->state);

  double sx, sy;
  wlr_surface *surface;
  View *view = desktop_view_at(cursor_->x, cursor_->y, &surface, &sx, &sy);

  if (event->state == WLR_BUTTON_RELEASED) {
    grab_state_.CursorMode = WM_CURSOR_PASSTHROUGH;
    return;
  }

  if (view != NULL) {
    focus_view(view);
  }
}

void Server::destroy_view(View *view) {
  auto condition = [view](auto &el) { return el.get() == view; };
  auto result = std::find_if(views_.begin(), views_.end(), condition);
  if (result != views_.end()) {
    views_.erase(result);
  }
}

void Server::new_xdg_surface_notify(wl_listener *listener, void *data) {
  auto xdg_surface = static_cast<wlr_xdg_surface*>(data);
  if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
    return;
  }

  Server *server = wl_container_of(listener, server, new_xdg_surface);
  auto view = std::make_shared<View>(server, xdg_surface, server->cursor_, server->layout_, server->seat_);
  server->views_.push_back(view);
}

void Server::maximize_view(View *view) {
  view->maximize();

  wlr_output* output = wlr_output_layout_output_at(layout_,
    cursor_->x, cursor_->y);

  wlr_box *output_box = wlr_output_layout_get_box(layout_, output);
  view->resize(output_box->width, output_box->height);

  view->x = output_box->x;
  view->y = output_box->y;
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
}

void Server::dock_right() {
  if (views_.empty()) return;

  auto &view = views_.front();
  view->tile_right();
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
  Server *server = wl_container_of(listener, server, request_set_primary_selection);
  auto event = static_cast<wlr_seat_request_set_primary_selection_event*>(data);
  server->on_set_primary_selection(event);
}

static void handle_request_set_selection(struct wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, request_set_selection);
  auto event = static_cast<wlr_seat_request_set_selection_event*>(data);
  server->on_set_selection(event);
}

void Server::run() {
  // wlr_log_init(WLR_DEBUG, NULL);

  init_keybindings();

  display_ = wl_display_create();
  backend_ = wlr_backend_autocreate(display_, NULL);
  renderer_ = wlr_backend_get_renderer(backend_);
  wlr_renderer_init_wl_display(renderer_, display_);

  wlr_compositor_create(display_, renderer_);
  wlr_data_device_manager_create(display_);

  layout_ = wlr_output_layout_create();

  new_output.notify = new_output_notify;
  wl_signal_add(&backend_->events.new_output, &new_output);

  xdg_shell_ = wlr_xdg_shell_create(display_);
  new_xdg_surface.notify = new_xdg_surface_notify;
  wl_signal_add(&xdg_shell_->events.new_surface, &new_xdg_surface);

  cursor_ = wlr_cursor_create();
  wlr_cursor_attach_output_layout(cursor_, layout_);

  cursor_manager_ = wlr_xcursor_manager_create(NULL, 24);

  cursor_motion.notify = Server::cursor_motion_notify;
  wl_signal_add(&cursor_->events.motion, &cursor_motion);

  cursor_motion_absolute.notify = Server::cursor_motion_absolute_notify;
  wl_signal_add(&cursor_->events.motion_absolute, &cursor_motion_absolute);

  cursor_button.notify = cursor_button_notify;
  wl_signal_add(&cursor_->events.button, &cursor_button);

  cursor_axis.notify = cursor_axis_notify;
  wl_signal_add(&cursor_->events.axis, &cursor_axis);

  cursor_frame.notify = cursor_frame_notify;
  wl_signal_add(&cursor_->events.frame, &cursor_frame);

  new_input.notify = new_input_notify;
  wl_signal_add(&backend_->events.new_input, &new_input);

  seat_ = wlr_seat_create(display_, "seat0");

  request_cursor.notify = seat_request_cursor_notify;
  wl_signal_add(&seat_->events.request_set_cursor, &request_cursor);

  wlr_data_control_manager_v1_create(display_);
  wlr_primary_selection_v1_device_manager_create(display_);

  request_set_selection.notify = handle_request_set_selection;
  wl_signal_add(&seat_->events.request_set_selection, &request_set_selection);

  request_set_primary_selection.notify = handle_request_set_primary_selection;
  wl_signal_add(&seat_->events.request_set_primary_selection, &request_set_primary_selection);

  const char *socket = wl_display_add_socket_auto(display_);
  if (!socket) {
    wlr_backend_destroy(backend_);
    return;
  }

  if (!wlr_backend_start(backend_)) {
    wlr_backend_destroy(backend_);
    wl_display_destroy(display_);
    return;
  }

  setenv("WAYLAND_DISPLAY", socket, true);

  std::clog << "Wayland WAYLAND_DISPLAY=" << socket << std::endl;

  wl_display_run(display_);
}

void Server::destroy() {
  std::clog << "Quitting!" << std::endl;

  wlr_xcursor_manager_destroy(cursor_manager_);
  wlr_cursor_destroy(cursor_);
  wl_display_destroy_clients(display_);
  wlr_backend_destroy(backend_);
  wlr_output_layout_destroy(layout_);
  wl_display_destroy(display_);
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
  auto lambda = [name](auto &output) -> bool { return output->is_named(name); };
  auto it = std::find_if(outputs_.begin(), outputs_.end(), lambda);
  if (it == outputs_.end()) {
    return;
  }

  auto &output = (*it);
  output->enable(enabled);
}

static void keyboard_modifiers_notify(wl_listener *listener, void *data) {
  Keyboard *keyboard = wl_container_of(listener, keyboard, modifiers);
  wlr_seat_set_keyboard(keyboard->seat, keyboard->device);
  wlr_seat_keyboard_notify_modifiers(keyboard->seat,
    &keyboard->device->keyboard->modifiers);
}

static void keyboard_key_notify(wl_listener *listener, void *data) {
  Keyboard *keyboard = wl_container_of(listener, keyboard, key);
  Server *server = keyboard->server;
  auto event = static_cast<struct wlr_event_keyboard_key *>(data);

  uint32_t keycode = event->keycode + 8;
  const xkb_keysym_t *syms;
  int nsyms = xkb_state_key_get_syms(keyboard->device->keyboard->xkb_state, keycode, &syms);
  uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->device->keyboard);

  bool handled = server->handle_key(keycode, syms, nsyms, modifiers, event->state);

  if (!handled) {
    wlr_seat_set_keyboard(keyboard->seat, keyboard->device);
    wlr_seat_keyboard_notify_key(keyboard->seat, event->time_msec, event->keycode, event->state);
  }
}

void Server::new_keyboard(wlr_input_device *device) {
  auto keyboard = std::make_shared<Keyboard>();
  keyboard->server = this;
  keyboard->device = device;
  keyboard->seat = seat_;

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

  wlr_seat_set_keyboard(seat_, device);

  keyboards_.push_back(keyboard);

  capabilities_ |= WL_SEAT_CAPABILITY_KEYBOARD;
  wlr_seat_set_capabilities(seat_, capabilities_);
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

  wlr_cursor_attach_input_device(cursor_, device);
  capabilities_ |= WL_SEAT_CAPABILITY_POINTER;
  wlr_seat_set_capabilities(seat_, capabilities_);
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
  grab_state_.view->x = cursor_->x - grab_state_.x;
  grab_state_.view->y = cursor_->y - grab_state_.y;
}

void Server::process_cursor_resize(uint32_t time) {
  View *view = grab_state_.view;

  double dx = cursor_->x - grab_state_.cursor_x;
  double dy = cursor_->y - grab_state_.cursor_y;

  double x = view->x;
  double y = view->y;

  double width = grab_state_.width;
  double height = grab_state_.height;

  if (grab_state_.resize_edges & WLR_EDGE_TOP) {
    y = grab_state_.y + dy;
    height = grab_state_.height - dy;
  } else if (grab_state_.resize_edges & WLR_EDGE_BOTTOM) {
    height += dy;
  }

  if (grab_state_.resize_edges & WLR_EDGE_LEFT) {
    x = grab_state_.x + dx;
    width -= dx;
  } else if (grab_state_.resize_edges & WLR_EDGE_RIGHT) {
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
  if (grab_state_.CursorMode == WM_CURSOR_MOVE) {
    process_cursor_move(time);
    damage_outputs();
    return;
  } else if (grab_state_.CursorMode == WM_CURSOR_RESIZE) {
    process_cursor_resize(time);
    return;
  }

  /* Otherwise, find the view under the pointer and send the event along. */
  double sx, sy;
  wlr_surface *surface = NULL;
  View *view = desktop_view_at(cursor_->x, cursor_->y, &surface, &sx, &sy);

  if (!view) {
    /* If there's no view under the cursor, set the cursor image to a
     * default. This is what makes the cursor image appear when you move it
     * around the screen, not over any views. */
    wlr_xcursor_manager_set_cursor_image(cursor_manager_, "left_ptr", cursor_);
  }

  if (surface) {
    bool focus_changed = seat_->pointer_state.focused_surface != surface;
    /*
     * "Enter" the surface if necessary. This lets the client know that the
     * cursor has entered one of its surfaces.
     *
     * Note that this gives the surface "pointer focus", which is distinct
     * from keyboard focus. You get pointer focus by moving the pointer over
     * a window.
     */

    wlr_seat_pointer_notify_enter(seat_, surface, sx, sy);

    if (!focus_changed) {
      /* The enter event contains coordinates, so we only need to notify
       * on motion if the focus did not change. */
      wlr_seat_pointer_notify_motion(seat_, time, sx, sy);
    }
  } else {
    /* Clear pointer focus so future button events and such are not sent to
     * the last client to have the cursor over it. */
    wlr_seat_pointer_clear_focus(seat_);
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
    wlr_output* output = wlr_output_layout_output_at(layout_, cursor_->x, cursor_->y);
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
