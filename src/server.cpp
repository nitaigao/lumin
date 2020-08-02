#include "server.h"

#include <linux/input-event-codes.h>
#include <wlroots.h>
#include <xkbcommon/xkbcommon.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <set>
#include <vector>

#include "wlroots_platform.h"
#include "posix_os.h"

#include "cursor.h"
#include "keyboard.h"
#include "output.h"
#include "seat.h"
#include "settings.h"
#include "dbus/adapters/compositor.h"
#include "xdg_view.h"

#include "key_binding.h"
#include "display_config.h"

namespace lumin {

Server::~Server() {}

Server::Server()
{
  settings_ = std::make_unique<Settings>();
  platform_ = std::make_unique<WlRootsPlatform>();
  os_ = std::make_unique<PosixOS>();
  display_config_ = std::make_unique<DisplayConfig>();
}

Server::Server(
  std::unique_ptr<IPlatform>& platform,
  std::unique_ptr<IOS>& os,
  std::unique_ptr<IDisplayConfig>& display_config
)
  : platform_(std::move(platform))
  , os_(std::move(os))
  , display_config_(std::move(display_config))
{
}

std::vector<std::shared_ptr<View>> filter_unminimized_views(const std::vector<std::shared_ptr<View>>& views)
{
  std::vector<std::shared_ptr<View>> filtered_views;
  std::copy_if(views.begin(), views.end(), std::back_inserter(filtered_views), [](auto &view) {
    return !view->minimized;
  });
  return filtered_views;
}

std::vector<std::shared_ptr<View>> filter_mapped_views(const std::vector<std::shared_ptr<View>>& views)
{
  std::vector<std::shared_ptr<View>> filtered_views;
  std::copy_if(views.begin(), views.end(), std::back_inserter(filtered_views), [](auto &view) {
    return !view->deleted && view->mapped;
  });
  return filtered_views;
}

void Server::quit()
{
  platform_->terminate();
}

std::vector<std::string> Server::apps() const
{
  std::vector<std::string> apps;
  auto mapped_views = filter_mapped_views(views_);
  for (auto &view : mapped_views) {
    auto id = view->root()->id();
    if (id.empty()) {
      apps.push_back("unknown");
      continue;
    }
    auto it = std::find_if(apps.begin(), apps.end(), [id](auto &el) { return el == id; });
    if (it == apps.end()) {
      apps.push_back(id);
    }
  }
  return apps;
}

void Server::damage_outputs()
{
  for (auto &output : outputs_) {
    output->take_whole_damage();
  }
}

void Server::damage_output(View *view)
{
  for (auto &output : outputs_) {
    output->take_damage(view);
  }
}

void Server::dbus_thread(Server *server)
{
  DBus::BusDispatcher dispatcher;
  DBus::default_dispatcher = &dispatcher;
  DBus::Connection bus = DBus::Connection::SessionBus();

  bus.request_name("org.os.Compositor");

  server->endpoint_ = std::make_unique<CompositorEndpoint>(bus, server);

  dispatcher.enter();
}

int Server::add_keybinding(int key_code, int modifiers, int state)
{
  unsigned int id = 0;
  bool exists = false;
  for (auto &pair : key_bindings) {
    if (pair.second.key_code_ == key_code && pair.second.modifiers_ == modifiers &&
      pair.second.state_ == state) {
      exists = true;
      id = pair.first;
      break;
    }
  }

  if (exists) {
    return id;
  }

  if (!key_bindings.empty()) {
    auto it = key_bindings.end();
    id = (*it).first + 1;
  }

  KeyBinding key_binding(key_code, modifiers, state);
  key_bindings.insert(std::make_pair(id, key_binding));

  return id;
}

bool Server::key(uint32_t keycode, uint32_t modifiers, int state)
{
  // Global shortcut to quit the compositor
  if (keycode == KEY_BACKSPACE && modifiers == (WLR_MODIFIER_CTRL | WLR_MODIFIER_ALT)) {
    quit();
    return true;
  }

  for (auto &pair : key_bindings) {
    bool matched = pair.second.matches(modifiers, keycode, (wlr_key_state)state);
    if (matched) {
      endpoint_->Shortcut(pair.first);
      return true;
    }
  }

  return false;
}

void Server::focus_app(const std::string& app_id)
{
  auto mapped_views = filter_mapped_views(views_);
  auto condition = [app_id](auto &el) { return el->id() == app_id; };
  auto result = std::find_if(mapped_views.begin(), mapped_views.end(), condition);
  if (result == mapped_views.end()) return;
  auto view = (*result).get();
  view->focus();
}

void Server::view_focused(View *view)
{
  auto seat = platform_->seat();
  wlr_surface *prev_surface = seat->keyboard_focused_surface();

  if (view->has_surface(prev_surface)) {
    return;
  }

  if (view->steals_focus() && prev_surface != nullptr) {
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
}

void Server::purge_deleted_outputs(void *data)
{
  Server *server = static_cast<Server*>(data);
  std::erase_if(server->outputs_, [](const auto &el) { return el.get()->deleted(); });
}

void Server::output_destroyed(Output *output)
{
  auto condition = [output](auto &el) { return el.get() == output; };
  auto result = std::find_if(outputs_.begin(), outputs_.end(), condition);
  if (result != outputs_.end()) {
    (*result)->mark_deleted();
  }

  platform_->add_idle(&Server::purge_deleted_outputs, this);
}

void Server::output_frame(Output *output)
{
  auto mapped_views = filter_mapped_views(views_);
  auto unminimized_views = filter_unminimized_views(mapped_views);
  output->send_enter(unminimized_views);
  output->render(unminimized_views);
}

void Server::output_mode(Output *output)
{
  if (!output->primary()) {
    return;
  }
}

void Server::purge_deleted_views(void *data)
{
  Server *server = static_cast<Server*>(data);
  std::erase_if(server->views_, [](const auto &el) { return el.get()->deleted; });
}

void Server::view_damaged(View *view)
{
  damage_output(view);
}

void Server::view_destroyed(View *view)
{
  auto condition = [view](auto &el) { return el.get() == view; };
  auto result = std::find_if(views_.begin(), views_.end(), condition);
  if (result != views_.end()) {
    (*result)->deleted = true;
  }
  platform_->add_idle(&Server::purge_deleted_views, this);
}

void Server::view_moved(View *view)
{
  damage_outputs();
}

void Server::view_mapped(View *view)
{
  position_view(view);
  view->focus();
  damage_output(view);
}

void Server::view_unmapped(View *view)
{
  focus_top();
  damage_outputs();
}

void Server::keyboard_key(uint32_t time_msec, uint32_t keycode, uint32_t modifiers, int state)
{
  auto seat = platform_->seat();
  bool handled = key(keycode, modifiers, state);

  if (!handled) {
    seat->keyboard_notify_key(time_msec, keycode, state);
  }
}

void Server::lid_switch(bool enabled)
{
  const std::vector<std::string> laptop_screen_names = { "eDP-1", "LVDS1" };

  for (auto &laptop_screen_name : laptop_screen_names) {
    enable_output(laptop_screen_name, enabled);
  }
}

void Server::cursor_button(Cursor *cursor, int x, int y)
{
  double sx, sy;
  wlr_surface *surface;
  View *view = desktop_view_at(x, y, &surface, &sx, &sy);

  if (view == nullptr) {
    return;
  }

  view->focus();
}

void Server::view_minimized(View *view)
{
  auto condition = [view](auto &el) { return el.get() == view; };
  auto result = std::find_if(views_.begin(), views_.end(), condition);
  if (result != views_.end()) {
    auto resultValue = *result;
    views_.erase(result);
    views_.push_back(resultValue);
  }

  focus_top();
}

void Server::maximize_view(View *view)
{
  auto cursor = platform_->cursor();
  view->maximize();

  auto output = platform_->output_at(cursor->x(), cursor->y());
  wlr_box *output_box = output->box();

  view->resize(output_box->width, output_box->height);
  view->move(output_box->x, output_box->y);
}

void Server::focus_top()
{
  auto mapped_views = filter_mapped_views(views_);
  auto unminimized_views = filter_unminimized_views(mapped_views);
  if (unminimized_views.empty()) return;

  auto top_view = unminimized_views.front();
  top_view->focus();
}

void Server::minimize_top()
{
  auto mapped_views = filter_mapped_views(views_);
  if (mapped_views.empty()) return;

  auto top_view = mapped_views.front();
  top_view->minimize();
}

void Server::toggle_maximize()
{
  auto mapped_views = filter_mapped_views(views_);
  if (mapped_views.empty()) return;

  auto top_view = mapped_views.front();
  top_view->toggle_maximized();
}

void Server::dock_left()
{
  auto mapped_views = filter_mapped_views(views_);
  if (mapped_views.empty()) return;

  auto top_view = mapped_views.front();
  top_view->tile_left();
}

void Server::dock_right()
{
  auto mapped_views = filter_mapped_views(views_);
  if (mapped_views.empty()) return;

  auto top_view = mapped_views.front();
  top_view->tile_right();
}

void Server::output_created(const std::shared_ptr<Output>& output)
{
  outputs_.push_back(output);

  output->on_destroy.connect_member(this, &Server::output_destroyed);
  output->on_frame.connect_member(this, &Server::output_frame);
  output->on_mode.connect_member(this, &Server::output_mode);
  output->on_connect.connect_member(this, &Server::output_connected);
  output->on_disconnect.connect_member(this, &Server::output_disconnected);

  output->set_connected(true);
}

void Server::view_created(const std::shared_ptr<View>& view)
{
  spdlog::debug("{} view launched", view->id());

  view->on_map.connect_member(this, &Server::view_mapped);
  view->on_unmap.connect_member(this, &Server::view_unmapped);
  view->on_minimize.connect_member(this, &Server::view_minimized);
  view->on_damage.connect_member(this, &Server::view_damaged);
  view->on_destroy.connect_member(this, &Server::view_destroyed);
  view->on_move.connect_member(this, &Server::view_moved);
  view->on_focus.connect_member(this, &Server::view_focused);

  views_.push_back(view);
}

void Server::cursor_motion(Cursor* cursor, int x, int y, uint32_t time)
{
  /* Otherwise, find the view under the pointer and send the event along. */
  double sx, sy;
  wlr_surface *surface = NULL;
  View *view = desktop_view_at(x, y, &surface, &sx, &sy);

  if (!view) {
    cursor->set_image("left_ptr");
  }

  auto seat = platform_->seat();

  if (surface) {
    bool focus_changed = seat->pointer_focused_surface() != surface;
    /*
     * "Enter" the surface if necessary. This lets the client know that the
     * cursor has entered one of its surfaces.
     *
     * Note that this gives the surface "pointer focus", which is distinct
     * from keyboard focus. You get pointer focus by moving the pointer over
     * a window.
     */
    seat->pointer_notify_enter(surface, sx, sy);

    if (!focus_changed) {
      /* The enter event contains coordinates, so we only need to notify
       * on motion if the focus did not change. */
      seat->pointer_motion(time, sx, sy);
    }
  } else {
    /* Clear pointer focus so future button events and such are not sent to
     * the last client to have the cursor over it. */
    seat->pointer_clear_focus();
  }
}

bool Server::init()
{
  spdlog::set_level(spdlog::level::debug);

  auto platform_init_result = platform_->init();
  if (!platform_init_result) {
    return false;
  }

  platform_->on_new_output.connect_member(this, &Server::output_created);
  platform_->on_new_view.connect_member(this, &Server::view_created);
  platform_->on_new_keyboard.connect_member(this, &Server::keyboard_created);
  platform_->on_cursor_motion.connect_member(this, &Server::cursor_motion);
  platform_->on_cursor_button.connect_member(this, &Server::cursor_button);
  platform_->on_lid_switch.connect_member(this, &Server::lid_switch);

  bool platform_start_result = platform_->start();
  if (!platform_start_result) {
    return false;
  }

  os_->set_env("MOZ_ENABLE_WAYLAND", "1");
  os_->set_env("QT_QPA_PLATFORM", "wayland");
  os_->set_env("QT_QPA_PLATFORMTHEME", "gnome");
  os_->set_env("XDG_CURRENT_DESKTOP", "sway");
  os_->set_env("XDG_SESSION_TYPE", "wayland");

  dbus_ = std::thread(Server::dbus_thread, this);

  if (fork() == 0) {
    execl("/bin/sh", "/bin/sh", "-c", "lumin-menu", NULL);
  }

  if (fork() == 0) {
    execl("/bin/sh", "/bin/sh", "-c", "lumin-shell", NULL);
  }

  return true;
}

void Server::run()
{
  platform_->run();
}

void Server::destroy()
{
  spdlog::warn("quitting");

  platform_->destroy();
}

void Server::enable_output(const std::string& name, bool enabled)
{
  auto lambda = [name](auto &output) -> bool { return output->is_named(name); };
  auto it = std::find_if(outputs_.begin(), outputs_.end(), lambda);
  if (it == outputs_.end()) {
    return;
  }

  auto &output = (*it);
  output->set_connected(enabled);
}

void Server::output_disconnected(Output* output)
{
  output->set_enabled(false);
  output->remove_layout();
  output->commit();
}

void Server::output_connected(IOutput* output)
{
  auto cursor = platform_->cursor();
  auto display_config = display_config_->find_layout(outputs_);

  for (auto& output : outputs_) {
    auto config = display_config[output->id()];
    cursor->load_scale(config.scale);
    output->configure(config.scale, config.primary);
  }
}

void Server::keyboard_created(const std::shared_ptr<Keyboard>& keyboard)
{
  keyboard->on_key.connect_member(this, &Server::keyboard_key);
  keyboards_.push_back(keyboard);
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

View* Server::view_from_surface(wlr_surface *surface)
{
  for (auto &view : views_) {
    if (view->has_surface(surface)) {
      return view.get();
    }
  }
  return NULL;
}

Output* Server::primary_output() const
{
  auto condition = [](auto &el) { return el->primary(); };
  auto result = std::find_if(outputs_.begin(), outputs_.end(), condition);

  if (result == outputs_.end()) {
    spdlog::error("Failed to find primary output");
    return nullptr;
  }

  return (Output*)(*result).get();
}

void Server::position_view(View *view)
{
  if (view->is_menubar()) {
    auto output = primary_output();
    output->set_menubar(view);
    return;
  }

  bool is_root = view->is_root();
  auto cursor = platform_->cursor();
  if (is_root) {
    Output *output = platform_->output_at(cursor->x(), cursor->y());
    output->add_view(view);
    return;
  }

  wlr_box geometry;
  view->geometry(&geometry);

  View *parent_view = view->parent();

  if (!parent_view) {
    return;
  }

  wlr_box parent_geometry;
  parent_view->geometry(&parent_geometry);

  int inside_x = (parent_geometry.width - geometry.width) / 2.0;
  int inside_y = (parent_geometry.height - geometry.height) / 2.0;

  view->x = parent_view->x + inside_x;
  view->y = parent_view->y + inside_y;
}

}  // namespace lumin
