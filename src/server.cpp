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
#include "xdg_view.h"
#include "graph.h"

#include "display_config.h"

namespace lumin {

Server::~Server() {}

Server::Server()
{
  platform_ = std::make_shared<WlRootsPlatform>();
  os_ = std::make_shared<PosixOS>();
  display_config_ = std::make_shared<DisplayConfig>(os_);
  graph_ = std::make_shared<Graph>();
}

Server::Server(
  const std::shared_ptr<IPlatform>& platform,
  const std::shared_ptr<IOS>& os,
  const std::shared_ptr<IDisplayConfig>& display_config,
  const std::shared_ptr<ICursor>& cursor
)
  : platform_(platform)
  , os_(os)
  , display_config_(display_config)
  , cursor_(cursor)
{
}

void Server::quit()
{
  platform_->terminate();
}

std::vector<std::string> Server::apps() const
{
  std::vector<std::string> apps;
  auto mapped_views = graph_->mapped_views();
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

void Server::view_focused(View *view)
{
  auto seat = platform_->seat();
  wlr_surface *prev_surface = seat->keyboard_focused_surface();

  if (view->has_surface(prev_surface)) {
    return;
  }

  if (view->steals_focus() && prev_surface != nullptr) {
    View *previous_view = static_cast<View*>(prev_surface->data);
    previous_view->unfocus();
  }

  graph_->bring_to_front(view);
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
  auto mapped_views = graph_->mapped_views();
  output->send_enter(mapped_views);

  for (auto& view : mapped_views) {
    if (view->is_menubar()) {
      view->set_size(output->width() / output->scale(), View::MENU_HEIGHT);
      view->move(output->x(), output->y());
    }
  }

  output->render(mapped_views);
}

void Server::output_mode(Output *output)
{
  if (!output->primary()) {
    return;
  }
}

void Server::purge_deleted_views(void *data)
{
  auto graph = static_cast<Graph*>(data);
  graph->remove_deleted_views();
}

void Server::view_damaged(View *view)
{
  damage_output(view);
}

void Server::view_destroyed(View *view)
{
  view->deleted = true;
  platform_->add_idle(&Server::purge_deleted_views, graph_.get());
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

void Server::keyboard_key(uint time_msec, uint keycode, uint modifiers, int state)
{
  auto seat = platform_->seat();
  bool handled = false;

  on_key.emit(keycode, modifiers, state, &handled);

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

void Server::cursor_button(ICursor *cursor, int x, int y)
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
  graph_->bring_to_front(view);
  focus_top();
}

void Server::focus_top()
{
  auto mapped_views = graph_->mapped_views();
  if (mapped_views.empty()) return;

  auto top_view = mapped_views.front();
  top_view->focus();
}

void Server::minimize_top()
{
  auto mapped_views = graph_->mapped_views();
  if (mapped_views.empty()) return;

  auto top_view = mapped_views.front();
  top_view->minimize();
}

void Server::maximize_top()
{
  auto mapped_views = graph_->mapped_views();
  if (mapped_views.empty()) return;

  auto top_view = mapped_views.front();
  top_view->toggle_maximized();
}

void Server::dock_top_left()
{
  auto mapped_views = graph_->mapped_views();
  if (mapped_views.empty()) return;

  auto top_view = mapped_views.front();
  top_view->tile_left();
}

void Server::dock_top_right()
{
  auto mapped_views = graph_->mapped_views();
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
  output->on_connect.connect_member(this, &Server::outputs_changed);
  output->on_disconnect.connect_member(this, &Server::outputs_changed);

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

  graph_->add_view(view);
}

void Server::cursor_motion(ICursor* cursor, int x, int y, uint32_t time)
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

  cursor_ = platform_->cursor();

  platform_->on_new_output.connect_member(this, &Server::output_created);
  platform_->on_new_view.connect_member(this, &Server::view_created);
  platform_->on_new_keyboard.connect_member(this, &Server::keyboard_created);
  platform_->on_lid_switch.connect_member(this, &Server::lid_switch);

  cursor_->on_button.connect_member(this, &Server::cursor_button);
  cursor_->on_move.connect_member(this, &Server::cursor_motion);

  bool platform_start_result = platform_->start();
  if (!platform_start_result) {
    return false;
  }

  os_->set_env("MOZ_ENABLE_WAYLAND", "1");
  os_->set_env("QT_QPA_PLATFORM", "wayland");
  os_->set_env("QT_QPA_PLATFORMTHEME", "gnome");
  os_->set_env("XDG_CURRENT_DESKTOP", "sway");
  os_->set_env("XDG_SESSION_TYPE", "wayland");

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

void Server::outputs_changed(IOutput* _output)
{
  auto display_config = display_config_->find_layout(outputs_);
  for (auto& configKV : display_config) {
    auto name = configKV.first;

    auto it = std::find_if(outputs_.begin(), outputs_.end(),
      [name](auto &output) -> bool { return name.compare(output->id()) == 0; });

    if (it == outputs_.end()) continue;

    auto output = (*it);
    auto config = configKV.second;
    cursor_->load_scale(config.scale);
    output->configure(config.scale, config.primary, config.enabled, config.x, config.y);
  }
}

void Server::keyboard_created(const std::shared_ptr<Keyboard>& keyboard)
{
  keyboard->on_key.connect_member(this, &Server::keyboard_key);
  keyboards_.push_back(keyboard);
}

View* Server::desktop_view_at(double lx, double ly,
  wlr_surface **surface, double *sx, double *sy)
{
  auto mapped_views = graph_->mapped_views();
  for (auto &view : mapped_views) {
    if (view->view_at(lx, ly, surface, sx, sy)) {
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
  bool is_root = view->is_root();
  if (is_root) {
    Output *output = platform_->output_at(cursor_->x(), cursor_->y());
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
