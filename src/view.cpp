#include "view.h"

#include "wlroots.h"

#include "server.h"
#include "output.h"
#include "cursor_mode.h"

const int DEFAULT_MINIMUM_WIDTH = 800;
const int DEFAULT_MINIMUM_HEIGHT = 600;

View::View(Server *server_, wlr_xdg_surface *surface)
  : mapped(false)
  , x(0)
  , y(0)
  , state(WM_WINDOW_STATE_WINDOW)
  , saved_state_({
    .width = DEFAULT_MINIMUM_WIDTH,
    .height = DEFAULT_MINIMUM_HEIGHT,
    .x = 0,
    .y = 0 })
  , server(server_)
  , xdg_surface(surface)
{ }

void View::destroyed() {
  server->destroy_view(this);
}

uint View::min_width() const {
  return xdg_surface->toplevel->current.min_width;
}

uint View::min_height() const {
  return xdg_surface->toplevel->current.min_height;
}

bool View::windowed() const {
  bool windowed = state == WM_WINDOW_STATE_WINDOW;
  return windowed;
}

bool View::tiled() const {
  bool tiled = state == WM_WINDOW_STATE_TILED;
  return tiled;
}

bool View::maximized() const {
  bool maximised = state == WM_WINDOW_STATE_MAXIMIZED;
  return maximised;
}

void View::toggle_maximized() {
  bool is_maximised = maximized();

  if (is_maximised) {
    window();
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

void View::begin_interactive(enum CursorMode mode, uint32_t edges) {
  server->begin_interactive(this, mode, edges);
}

void View::map_view() {
  mapped = true;
  server->position_view(this);
  focus();
  server->damage_outputs();
}

void View::unmap_view() {
  mapped = false;
  server->focus_top();
  server->damage_outputs();
}

void View::committed() {
  server->damage_outputs();
}

void View::for_each_surface(wlr_surface_iterator_func_t iterator, void *data) const {
  if (xdg_surface->surface == NULL) {
    return;
  }
  wlr_xdg_surface_for_each_surface(xdg_surface, iterator, data);
}

void View::focus() {
  server->focus_view(this);
}

void View::unfocus() {
  wlr_xdg_toplevel_set_activated(xdg_surface, false);
}

void View::geometry(struct wlr_box *box) const {
  wlr_surface_get_extends(xdg_surface->surface, box);

  if (!xdg_surface->geometry.width) {
    return;
  }

  wlr_box_intersection(&xdg_surface->geometry, box, box);
}

const wlr_surface* View::surface() const {
  return xdg_surface->surface;
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

void View::tile(int edges) {
  if (tiled()) {
    return;
  }

  save_geometry();

  bool is_maximized = maximized();
  if (is_maximized) {
    wlr_xdg_toplevel_set_maximized(xdg_surface, false);
  }

  wlr_xdg_toplevel_set_tiled(xdg_surface, edges);
  state = WM_WINDOW_STATE_TILED;
}

void View::maximize() {
  if (maximized()) {
    return;
  }

  server->maximize_view(this);
}

void View::maxi() {
  save_geometry();

  bool is_tiled = tiled();
  if (is_tiled) {
    wlr_xdg_toplevel_set_tiled(xdg_surface, WLR_EDGE_NONE);
  }

  wlr_xdg_toplevel_set_maximized(xdg_surface, true);

  state = WM_WINDOW_STATE_MAXIMIZED;
}

void View::grabi(int grab_x) {
  if (windowed()) {
    return;
  }

  if (tiled()) {
    wlr_xdg_toplevel_set_tiled(xdg_surface, WLR_EDGE_NONE);
  }

  if (maximized()) {
    wlr_xdg_toplevel_set_maximized(xdg_surface, false);
  }

  wlr_xdg_toplevel_set_size(xdg_surface, saved_state_.width, saved_state_.height);

  wlr_box geometry;
  extents(&geometry);

  float surface_x = grab_x - x;
  float x_percentage = surface_x / geometry.width;
  float desired_x = saved_state_.width * x_percentage;
  x = grab_x - desired_x;

  server->damage_outputs();
  state = WM_WINDOW_STATE_WINDOW;
}

void View::grab() {
  server->view_grabbed(this);
}

void View::window() {
  if (windowed()) {
    return;
  }

  if (tiled()) {
    wlr_xdg_toplevel_set_tiled(xdg_surface, WLR_EDGE_NONE);
  }

  if (maximized()) {
    wlr_xdg_toplevel_set_maximized(xdg_surface, false);
  }

  wlr_xdg_toplevel_set_size(xdg_surface, saved_state_.width, saved_state_.height);

  x = saved_state_.x;
  y = saved_state_.y;

  server->damage_outputs();
  state = WM_WINDOW_STATE_WINDOW;
}

void View::extents(wlr_box *box) const {
  wlr_xdg_surface_get_geometry(xdg_surface, box);
}

void View::activate() {
  wlr_xdg_toplevel_set_activated(xdg_surface, true);
}

void View::notify_keyboard_enter(wlr_seat *seat) {
  wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);
  wlr_seat_keyboard_notify_enter(seat, xdg_surface->surface,
    keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
}

void View::resize(double width, double height) {
  wlr_xdg_toplevel_set_size(xdg_surface, width, height);
}

wlr_surface* View::surface_at(double sx, double sy, double *sub_x, double *sub_y) {
  return wlr_xdg_surface_surface_at(xdg_surface, sx, sy, sub_x, sub_y);
}

View* View::parent() const {
  return server->view_from_surface(xdg_surface->toplevel->parent->surface);
}

bool View::is_child() const {
  bool is_child = xdg_surface->toplevel->parent != NULL;
  return is_child;
}
