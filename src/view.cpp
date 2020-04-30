#include "view.h"

#include <wlroots.h>
#include <spdlog/spdlog.h>

#include "cursor_mode.h"
#include "cursor.h"
#include "output.h"
#include "seat.h"
#include "server.h"

const int DEFAULT_MINIMUM_WIDTH = 800;
const int DEFAULT_MINIMUM_HEIGHT = 600;
const int MENU_HEIGHT = 25;

namespace lumin {

View::View(Server *server_, wlr_xdg_surface *surface,
  Cursor *cursor, wlr_output_layout *layout, Seat *seat)
  : mapped(false)
  , x(0)
  , y(0)
  , minimized(false)
  , state(WM_WINDOW_STATE_WINDOW)
  , saved_state_({
    .width = DEFAULT_MINIMUM_WIDTH,
    .height = DEFAULT_MINIMUM_HEIGHT,
    .x = 0,
    .y = 0 })
  , server(server_)
  , cursor_(cursor)
  , layout_(layout)
  , seat_(seat)
  , xdg_surface_(surface)
{
  map.notify = View::xdg_surface_map_notify;
  wl_signal_add(&xdg_surface_->events.map, &map);

  unmap.notify = View::xdg_surface_unmap_notify;
  wl_signal_add(&xdg_surface_->events.unmap, &unmap);

  destroy.notify = View::xdg_surface_destroy_notify;
  wl_signal_add(&xdg_surface_->events.destroy, &destroy);

  commit.notify = View::xdg_surface_commit_notify;
  wl_signal_add(&xdg_surface_->surface->events.commit, &commit);

  new_subsurface.notify = View::new_subsurface_notify;
  wl_signal_add(&xdg_surface_->surface->events.new_subsurface, &new_subsurface);

  new_popup.notify = View::new_popup_notify;
  wl_signal_add(&xdg_surface_->events.new_popup, &new_popup);

  struct wlr_xdg_toplevel *toplevel = xdg_surface_->toplevel;

  request_move.notify = View::xdg_toplevel_request_move_notify;
  wl_signal_add(&toplevel->events.request_move, &request_move);

  request_resize.notify = View::xdg_toplevel_request_resize_notify;
  wl_signal_add(&toplevel->events.request_resize, &request_resize);

  request_maximize.notify = View::xdg_toplevel_request_maximize_notify;
  wl_signal_add(&toplevel->events.request_maximize, &request_maximize);

  request_minimize.notify = View::xdg_toplevel_request_minimize_notify;
  wl_signal_add(&toplevel->events.request_minimize, &request_minimize);

  request_fullscreen.notify = View::xdg_toplevel_request_fullscreen_notify;
  wl_signal_add(&toplevel->events.request_fullscreen, &request_fullscreen);

  set_app_id.notify = View::xdg_toplevel_set_app_id_notify;
  wl_signal_add(&toplevel->events.set_app_id, &set_app_id);
}

std::string View::id() const {
  if (xdg_surface_->toplevel->app_id == nullptr) {
    return "";
  }

  return xdg_surface_->toplevel->app_id;
}

std::string View::title() const {
  return xdg_surface_->toplevel->title;
}

uint View::min_width() const {
  return xdg_surface_->toplevel->current.min_width;
}

uint View::min_height() const {
  return xdg_surface_->toplevel->current.min_height;
}

bool View::windowed() const {
  bool windowed = state == WM_WINDOW_STATE_WINDOW;
  return windowed;
}

void surface_send_enter(wlr_surface *surface, int sx, int sy, void *data) {
  auto output = static_cast<wlr_output*>(data);
  wlr_surface_send_enter(surface, output);
}

void View::enter(const Output* output) {
  for_each_surface(surface_send_enter, output->wlr_output);
}

bool View::tiled() const {
  bool tiled = state == WM_WINDOW_STATE_TILED;
  return tiled;
}

bool View::fullscreen() const {
  bool fullscreen = state == WM_WINDOW_STATE_FULLSCREEN;
  return fullscreen;
}

bool View::maximized() const {
  bool maximised = state == WM_WINDOW_STATE_MAXIMIZED;
  return maximised;
}

void View::toggle_maximized() {
  bool is_maximised = maximized();

  if (is_maximised) {
    windowize();
  } else {
    maximize();
  }
}

bool View::view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy) {
  if (!mapped) {
    return false;
  }
  /*
   * XDG toplevels may have nested surfaces, such as popup windows for context
   * menus or tooltips. This function tests if any of those are underneath the
   * coordinates lx and ly (in output Layout Coordinates). If so, it sets the
   * surface pointer to that wlr_surface and the sx and sy coordinates to the
   * coordinates relative to that surface's top-left corner.
   */
  double view_sx = lx - x;
  double view_sy = ly - y;

  double _sx, _sy;
  wlr_surface *_surface = surface_at(view_sx, view_sy, &_sx, &_sy);

  if (_surface != NULL) {
    *sx = _sx;
    *sy = _sy;
    *surface = _surface;
    return true;
  }

  return false;
}

void View::map_view() {
  mapped = true;
}

void View::unmap_view() {
  mapped = false;
}

void View::for_each_surface(wlr_surface_iterator_func_t iterator, void *data) const {
  if (xdg_surface_->surface == NULL) {
    return;
  }
  wlr_xdg_surface_for_each_surface(xdg_surface_, iterator, data);
}

void View::focus() {
  minimized = false;
  activate();
  seat_->keyboard_notify_enter(xdg_surface_->surface);
}

void View::unfocus() {
  if (is_always_focused()) {
    return;
  }

  wlr_xdg_toplevel_set_activated(xdg_surface_, false);
}

bool View::steals_focus() const {
  return !(is_menubar() || is_launcher());
}

bool View::is_always_focused() const {
  return is_menubar() || is_launcher();
}

ViewLayer View::layer() const {
  if (is_menubar() || is_launcher()) {
    return VIEW_LAYER_OVERLAY;
  }
  return VIEW_LAYER_TOP;
}

bool View::is_menubar() const {
  bool result = id().compare("org.os.Menu") == 0;
  return result;
}

bool View::is_launcher() const {
  bool result = id().compare("org.os.Launcher") == 0;
  return result;
}

bool View::is_shell() const {
  bool result = id().compare("org.os.Shell") == 0;
  return result;
}

void View::geometry(struct wlr_box *box) const {
  wlr_surface_get_extends(xdg_surface_->surface, box);

  if (!xdg_surface_->geometry.width) {
    return;
  }

  wlr_box_intersection(&xdg_surface_->geometry, box, box);
}

bool View::has_surface(const wlr_surface *surface) const {
  return xdg_surface_->surface == surface;
}

void View::save_geometry() {
  if (state == WM_WINDOW_STATE_WINDOW) {
    wlr_box geometry;
    extents(&geometry);

    if (geometry.width != 0) {
      saved_state_.width = geometry.width;
      saved_state_.height = geometry.height;
    }

    saved_state_.x = x;
    saved_state_.y = y;
  }
}

void View::tile_left() {
  tile(WLR_EDGE_LEFT);

  wlr_box box;
  extents(&box);

  int corner_x = x + box.x + (box.width / 2.0f);
  int corner_y = y + box.y + (box.height / 2.0f);
  wlr_output* output = wlr_output_layout_output_at(layout_, corner_x, corner_y);

  int width = (output->width / 2.0f) / output->scale;
  int height = output->height / output->scale;
  resize(width, height - MENU_HEIGHT);

  x = 0;
  y = MENU_HEIGHT;

  wlr_output_layout_output_coords(layout_, output, &x, &y);

  server->damage_outputs();
}

void View::tile_right() {
  tile(WLR_EDGE_RIGHT);

  wlr_box box;
  extents(&box);

  int corner_x = x + box.x + (box.width / 2.0f);
  int corner_y = y + box.y + (box.height / 2.0f);
  wlr_output* output = wlr_output_layout_output_at(layout_, corner_x, corner_y);

  y = MENU_HEIGHT;
  x = 0;

  wlr_output_layout_output_coords(layout_, output, &x, &y);

  // middle of the screen
  x += (output->width / 2.0f) / output->scale;

  int width = (output->width / 2.0f) / output->scale;
  int height = output->height / output->scale;

  resize(width, height);
  server->damage_outputs();
}

void View::tile(int edges) {
  if (tiled()) {
    return;
  }

  save_geometry();

  bool is_maximized = maximized();
  if (is_maximized) {
    wlr_xdg_toplevel_set_maximized(xdg_surface_, false);
  }

  wlr_xdg_toplevel_set_tiled(xdg_surface_, edges);
  state = WM_WINDOW_STATE_TILED;
}

void View::minimize() {
  minimized = true;
}

void View::maximize() {
  if (maximized()) {
    return;
  }

  save_geometry();

  bool is_tiled = tiled();
  if (is_tiled) {
    wlr_xdg_toplevel_set_tiled(xdg_surface_, WLR_EDGE_NONE);
  }

  wlr_xdg_toplevel_set_maximized(xdg_surface_, true);

  wlr_output* output = wlr_output_layout_output_at(layout_,
    cursor_->x(), cursor_->y());

  wlr_box *output_box = wlr_output_layout_get_box(layout_, output);
  resize(output_box->width, output_box->height - MENU_HEIGHT);

  x = output_box->x;
  y = output_box->y + MENU_HEIGHT;

  state = WM_WINDOW_STATE_MAXIMIZED;
}

void View::grab() {
  if (windowed()) {
    return;
  }

  if (tiled()) {
    wlr_xdg_toplevel_set_tiled(xdg_surface_, WLR_EDGE_NONE);
  }

  if (maximized()) {
    wlr_xdg_toplevel_set_maximized(xdg_surface_, false);
  }

  wlr_xdg_toplevel_set_size(xdg_surface_, saved_state_.width, saved_state_.height);

  wlr_box geometry;
  extents(&geometry);

  float surface_x = cursor_->x() - x;
  float x_percentage = surface_x / geometry.width;
  float desired_x = saved_state_.width * x_percentage;
  x = cursor_->x() - desired_x;

  server->damage_outputs();
  state = WM_WINDOW_STATE_WINDOW;
}

void View::windowize() {
  if (windowed()) {
    return;
  }

  if (tiled()) {
    wlr_xdg_toplevel_set_tiled(xdg_surface_, WLR_EDGE_NONE);
  }

  if (maximized()) {
    wlr_xdg_toplevel_set_maximized(xdg_surface_, false);
  }

  wlr_xdg_toplevel_set_size(xdg_surface_, saved_state_.width, saved_state_.height);

  x = saved_state_.x;
  y = saved_state_.y;

  server->damage_outputs();
  state = WM_WINDOW_STATE_WINDOW;
}

void View::extents(wlr_box *box) const {
  wlr_xdg_surface_get_geometry(xdg_surface_, box);
}

void View::activate() {
  wlr_xdg_toplevel_set_activated(xdg_surface_, true);
}

void View::resize(double width, double height) {
  wlr_xdg_toplevel_set_size(xdg_surface_, width, height);
}

wlr_surface* View::surface_at(double sx, double sy, double *sub_x, double *sub_y) {
  return wlr_xdg_surface_surface_at(xdg_surface_, sx, sy, sub_x, sub_y);
}

View* View::parent() const {
  return server->view_from_surface(xdg_surface_->toplevel->parent->surface);
}

bool View::is_child() const {
  bool is_child = xdg_surface_->toplevel->parent != NULL;
  return is_child;
}

const View* View::root() const {
  if (xdg_surface_->toplevel->parent == NULL) {
    return this;
  }
  return parent()->root();
}

void View::xdg_toplevel_request_move_notify(wl_listener *listener, void *data) {
  View *view = wl_container_of(listener, view, request_move);
  view->grab();
  view->cursor_->begin_interactive(view, WM_CURSOR_MOVE, WLR_EDGE_NONE);
}

void View::xdg_toplevel_request_resize_notify(wl_listener *listener, void *data) {
  auto event = static_cast<wlr_xdg_toplevel_resize_event*>(data);
  View *view = wl_container_of(listener, view, request_resize);
  view->cursor_->begin_interactive(view, WM_CURSOR_RESIZE, event->edges);
}

void View::xdg_toplevel_request_maximize_notify(wl_listener *listener, void *data) {
  View *view = wl_container_of(listener, view, request_maximize);
  view->toggle_maximized();
}

void View::xdg_toplevel_request_minimize_notify(wl_listener *listener, void *data) {
  View *view = wl_container_of(listener, view, request_minimize);
  view->server->minimize_view(view);
}

void View::xdg_toplevel_request_fullscreen_notify(wl_listener *listener, void *data) {
  auto event = static_cast<wlr_xdg_toplevel_set_fullscreen_event*>(data);
  View *view = wl_container_of(listener, view, request_fullscreen);
  view->state = event->fullscreen ? WM_WINDOW_STATE_FULLSCREEN : WM_WINDOW_STATE_WINDOW;
}

void View::xdg_surface_destroy_notify(wl_listener *listener, void *data) {
  View *view = wl_container_of(listener, view, destroy);
  view->server->destroy_view(view);
  view->server->damage_outputs();
}

void View::xdg_popup_subsurface_commit_notify(wl_listener *listener, void *data) {
  Subsurface *subsurface = wl_container_of(listener, subsurface, commit);
  subsurface->server->damage_output(subsurface->view);
}

void View::xdg_subsurface_commit_notify(wl_listener *listener, void *data) {
  Subsurface *subsurface = wl_container_of(listener, subsurface, commit);
  subsurface->server->damage_output(subsurface->view);
}

void View::xdg_popup_destroy_notify(wl_listener *listener, void *data) {
  Popup *popup = wl_container_of(listener, popup, destroy);
  popup->server->damage_outputs();
}

void View::xdg_popup_commit_notify(wl_listener *listener, void *data) {
  Popup *popup = wl_container_of(listener, popup, commit);
  popup->server->damage_output(popup->view);
}

void View::xdg_surface_commit_notify(wl_listener *listener, void *data) {
  View *view = wl_container_of(listener, view, commit);
  if (view->mapped) {
    view->server->damage_output(view);
  }
}

void View::new_popup_subsurface_notify(wl_listener *listener, void *data) {
  auto *wlr_subsurface_ = static_cast<wlr_subsurface*>(data);
  Popup *popup = wl_container_of(listener, popup, new_subsurface);

  auto subsurface = new Subsurface();
  subsurface->server = popup->server;
  subsurface->view = popup->view;

  subsurface->commit.notify = xdg_popup_subsurface_commit_notify;
  wl_signal_add(&wlr_subsurface_->surface->events.commit, &subsurface->commit);
}

void View::new_subsurface_notify(wl_listener *listener, void *data) {
  auto *wlr_subsurface_ = static_cast<wlr_subsurface*>(data);
  View *view = wl_container_of(listener, view, new_subsurface);

  auto subsurface = new Subsurface();
  subsurface->server = view->server;
  subsurface->view = view;

  subsurface->commit.notify = xdg_subsurface_commit_notify;
  wl_signal_add(&wlr_subsurface_->surface->events.commit, &subsurface->commit);
}

void View::new_popup_popup_notify(wl_listener *listener, void *data) {
  auto *xdg_popup = static_cast<wlr_xdg_popup*>(data);
  Popup *parent_popup = wl_container_of(listener, parent_popup, new_popup);

  auto popup = new Popup();
  popup->server = parent_popup->server;
  popup->view = parent_popup->view;

  popup->commit.notify = xdg_popup_commit_notify;
  wl_signal_add(&xdg_popup->base->surface->events.commit, &popup->commit);

  popup->destroy.notify = xdg_popup_destroy_notify;
  wl_signal_add(&xdg_popup->base->surface->events.destroy, &popup->destroy);

  popup->new_subsurface.notify = new_popup_subsurface_notify;
  wl_signal_add(&xdg_popup->base->surface->events.new_subsurface, &popup->new_subsurface);

  popup->new_popup.notify = new_popup_popup_notify;
  wl_signal_add(&xdg_popup->base->events.new_popup, &popup->new_popup);
}

void View::new_popup_notify(wl_listener *listener, void *data) {
  auto *xdg_popup = static_cast<wlr_xdg_popup*>(data);

  View *view = wl_container_of(listener, view, new_popup);

  auto popup = new Popup();
  popup->server = view->server;
  popup->view = view;

  popup->commit.notify = xdg_popup_commit_notify;
  wl_signal_add(&xdg_popup->base->surface->events.commit, &popup->commit);

  popup->destroy.notify = xdg_popup_destroy_notify;
  wl_signal_add(&xdg_popup->base->surface->events.destroy, &popup->destroy);

  popup->new_subsurface.notify = new_popup_subsurface_notify;
  wl_signal_add(&xdg_popup->base->surface->events.new_subsurface, &popup->new_subsurface);

  popup->new_popup.notify = new_popup_popup_notify;
  wl_signal_add(&xdg_popup->base->events.new_popup, &popup->new_popup);
}

void View::xdg_surface_map_notify(wl_listener *listener, void *data) {
  View *view = wl_container_of(listener, view, map);
  view->map_view();
  view->server->position_view(view);
  view->server->focus_view(view);
  view->server->damage_output(view);
}

void View::xdg_surface_unmap_notify(wl_listener *listener, void *data) {
  View *view = wl_container_of(listener, view, unmap);
  view->unmap_view();
  view->server->focus_top();
  view->server->damage_output(view);
}

void View::xdg_toplevel_set_app_id_notify(wl_listener *listener, void *data) {
  View *view = wl_container_of(listener, view, set_app_id);
}

}  // namespace lumin
