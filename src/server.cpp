#include "server.h"

#include <iostream>
#include <memory>
#include <vector>

#include <spdlog/spdlog.h>

#include <wlroots.h>
#include <xkbcommon/xkbcommon.h>

#include "cursor.h"
#include "keyboard.h"
#include "output.h"
#include "seat.h"
#include "settings.h"
#include "view.h"

#include "key_bindings/key_binding_cmd.h"
#include "key_bindings/key_binding_quit.h"
#include "key_bindings/key_binding_dock_right.h"
#include "key_bindings/key_binding_dock_left.h"
#include "key_bindings/key_binding_maximize.h"

namespace lumin {

Server::~Server() {
}

Server::Server() {
  settings_ = std::make_unique<Settings>();
}

void Server::quit() {
  wl_display_terminate(display_);
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

void Server::new_input_notify(wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, new_input);
  auto device = static_cast<struct wlr_input_device *>(data);

  switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD:
      server->new_keyboard(device);
      break;
    case WLR_INPUT_DEVICE_POINTER:
      server->cursor_->add_device(device);
      break;
    case WLR_INPUT_DEVICE_SWITCH:
      server->new_switch(device);
      break;
    default:
      break;
  }
}

void Server::focus_view(View *view) {
  wlr_surface *prev_surface = seat_->keyboard_focused_surface();

  if (view->has_surface(prev_surface)) {
    return;
  }

  if (prev_surface) {
    View *previous_view = view_from_surface(prev_surface);
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

void Server::add_output(const std::shared_ptr<Output>& output) {
  outputs_.push_back(output);
  output->init();
  apply_layout();
}

void Server::remove_output(Output *output) {
  spdlog::debug("{} disconnected", output->id());

  output->remove_layout();
  output->destroy();

  auto condition = [output](auto &el) { return el.get() == output; };
  auto result = std::find_if(outputs_.begin(), outputs_.end(), condition);
  if (result != outputs_.end()) {
    outputs_.erase(result);
  }

  apply_layout();
}

void Server::apply_layout() {
  auto layout = settings_->display_find_layout(outputs_);

  std::map<Output*, DisplaySetting> enabled_outputs;
  std::map<Output*, DisplaySetting> disabled_outputs;

  for (auto &output : outputs_) {
    DisplaySetting display = layout[output->id()];
    spdlog::debug("{} scale:{} x:{} y:{} enabled:{}", output->id(), display.scale, display.x, display.y, display.enabled);

    if (!output->disconnected() && display.enabled) {
      enabled_outputs.insert(std::make_pair(output.get(), display));
    } else {
      disabled_outputs.insert(std::make_pair(output.get(), display));
    }

    cursor_->load_scale(display.scale);
  }

  for (auto& [output, display] : enabled_outputs) {
    output->set_enabled(true);
    output->set_scale(display.scale);
    output->set_mode();
    output->commit();

    output->remove_layout();

    output->add_layout(display.x, display.y);

    if (display.primary) {
      int cursor_x = display.x + (output->wlr_output->width * 0.5f) / display.scale;
      int cursor_y = display.y + (output->wlr_output->height  * 0.5f) / display.scale;

      cursor_->warp(cursor_x, cursor_y);
    }
  }

  for (auto& [output, display] : disabled_outputs) {
    output->set_enabled(false);
    output->commit();

    output->remove_layout();
  }
}

void Server::new_output_notify(wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, new_output);
  auto wlr_output = static_cast<struct wlr_output*>(data);

  auto damage = wlr_output_damage_create(wlr_output);
  auto output = std::make_shared<Output>(server, wlr_output,
    server->renderer_, damage, server->layout_);

  spdlog::debug("{} connected", output->id());

  server->add_output(output);
}

void Server::destroy_view(View *view) {
  auto condition = [view](auto &el) { return el.get() == view; };
  auto result = std::find_if(views_.begin(), views_.end(), condition);
  if (result != views_.end()) {
    views_.erase(result);
  }
}

void Server::new_surface_notify(wl_listener *listener, void *data) {
  auto xdg_surface = static_cast<wlr_xdg_surface*>(data);
  if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
    return;
  }

  Server *server = wl_container_of(listener, server, new_surface);

  auto view = std::make_shared<View>(server, xdg_surface,
    server->cursor_.get(), server->layout_, server->seat_.get());

  server->views_.push_back(view);
}

void Server::maximize_view(View *view) {
  view->maximize();

  wlr_output* output = wlr_output_layout_output_at(layout_,
    cursor_->x(), cursor_->y());

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

void Server::run() {
  spdlog::set_level(spdlog::level::debug);

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

  new_surface.notify = new_surface_notify;
  wl_signal_add(&xdg_shell_->events.new_surface, &new_surface);

  new_input.notify = new_input_notify;
  wl_signal_add(&backend_->events.new_input, &new_input);

  wlr_seat *seat = wlr_seat_create(display_, "seat0");
  seat_ = std::make_unique<Seat>(this, seat);
  cursor_ = std::make_unique<Cursor>(this, layout_, seat_.get());
  seat_->set_pointer(cursor_.get());

  wlr_data_control_manager_v1_create(display_);
  wlr_primary_selection_v1_device_manager_create(display_);

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
  spdlog::info("WAYLAND_DISPLAY={}", socket);

  wl_display_run(display_);
}

void Server::destroy() {
  spdlog::warn("quitting");

  wl_display_destroy_clients(display_);
  wlr_backend_destroy(backend_);
  wlr_output_layout_destroy(layout_);
  wl_display_destroy(display_);
}

void Server::lid_notify(wl_listener *listener, void *data) {
  auto event = static_cast<wlr_event_switch_toggle *>(data);
  Server *server = wl_container_of(listener, server, lid);

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
  output->set_disconnected(!enabled);
  apply_layout();
}

void Server::new_keyboard(wlr_input_device *device) {
  auto keyboard = std::make_shared<Keyboard>(this, device, seat_.get());
  keyboard->setup();
  keyboards_.push_back(keyboard);
}

void Server::new_switch(wlr_input_device *device) {
  spdlog::debug("new_switch");
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

View* Server::view_from_surface(wlr_surface *surface) {
  for (auto &view : views_) {
    if (view->has_surface(surface)) {
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
    wlr_output* output = wlr_output_layout_output_at(layout_, cursor_->x(), cursor_->y());
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

}  // namespace lumin
