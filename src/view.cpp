#include "view.h"

#include <wlroots.h>
#include <spdlog/spdlog.h>

#include "cursor_mode.h"
#include "cursor.h"
#include "output.h"
#include "seat.h"

const int DEFAULT_MINIMUM_WIDTH = 800;
const int DEFAULT_MINIMUM_HEIGHT = 600;

namespace lumin {

View::View(ICursor *cursor, wlr_output_layout *layout, Seat *seat)
  : mapped(false)
  , x(0)
  , y(0)
  , minimized(false)
  , deleted(false)
  , state(WM_WINDOW_STATE_WINDOW)
  , saved_state_({
    .width = DEFAULT_MINIMUM_WIDTH,
    .height = DEFAULT_MINIMUM_HEIGHT,
    .x = 0,
    .y = 0 })
  , cursor_(cursor)
  , layout_(layout)
  , seat_(seat)
{

}

bool View::windowed() const
{
  bool windowed = state == WM_WINDOW_STATE_WINDOW;
  return windowed;
}

bool View::tiled() const
{
  bool tiled = state == WM_WINDOW_STATE_TILED;
  return tiled;
}

bool View::fullscreen() const
{
  bool fullscreen = state == WM_WINDOW_STATE_FULLSCREEN;
  return fullscreen;
}

bool View::maximized() const
{
  bool maximised = state == WM_WINDOW_STATE_MAXIMIZED;
  return maximised;
}

void View::toggle_maximized()
{
  bool is_maximised = maximized();

  if (is_maximised) {
    windowize();
  } else {
    maximize();
  }
}

bool View::view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy)
{
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

void View::minimize()
{
  minimized = true;
  on_minimize.emit(this);
}

void View::focus()
{
  minimized = false;

  on_focus.emit(this);

  activate();
  seat_->keyboard_notify_enter(surface());
}

void View::unfocus()
{
  if (is_always_focused()) {
    return;
  }

  deactivate();
}

bool View::steals_focus() const
{
  return !(is_menubar() || is_launcher() || is_shell());
}

bool View::is_always_focused() const
{
  return is_menubar() || is_launcher();
}

ViewLayer View::layer() const
{
  if (is_menubar() || is_launcher()) {
    return VIEW_LAYER_OVERLAY;
  }
  return VIEW_LAYER_TOP;
}

bool View::is_menubar() const
{
  bool result = id().compare("org.os.Menu") == 0;
  return result;
}

bool View::is_launcher() const
{
  bool result = id().compare("org.os.Launcher") == 0;
  return result;
}

bool View::is_shell() const
{
  bool result = id().compare("org.os.Shell") == 0;
  return result;
}

void View::save_geometry()
{
  if (state != WM_WINDOW_STATE_WINDOW) {
    return;
  }

  wlr_box box;
  geometry(&box);

  if (box.width != 0) {
    saved_state_.width = box.width;
    saved_state_.height = box.height;
  }

  saved_state_.x = x;
  saved_state_.y = y;
}

void View::tile_left()
{
  tile(WLR_EDGE_LEFT);

  wlr_box box;
  geometry(&box);

  int corner_x = x + box.x + (box.width / 2.0f);
  int corner_y = y + box.y + (box.height / 2.0f);
  wlr_output* output = wlr_output_layout_output_at(layout_, corner_x, corner_y);

  if (output == nullptr) {
    spdlog::warn("Failed to get output for tiling");
    return;
  }

  int width = (output->width / 2.0f) / output->scale;
  int height = output->height / output->scale;
  resize(width, height - MENU_HEIGHT);
  move(0, MENU_HEIGHT);

  wlr_output_layout_output_coords(layout_, output, &x, &y);
}

void View::tile_right()
{
  tile(WLR_EDGE_RIGHT);

  wlr_box box;
  geometry(&box);

  int corner_x = x + box.x + (box.width / 2.0f);
  int corner_y = y + box.y + (box.height / 2.0f);
  wlr_output* output = wlr_output_layout_output_at(layout_, corner_x, corner_y);

  if (output == nullptr) {
    spdlog::warn("Failed to get output for tiling");
    return;
  }

  int new_y = MENU_HEIGHT;
  int new_x = 0;

  wlr_output_layout_output_coords(layout_, output, &x, &y);

  // middle of the screen
  new_x += (output->width / 2.0f) / output->scale;

  int width = (output->width / 2.0f) / output->scale;
  int height = output->height / output->scale;
  resize(width, height - MENU_HEIGHT);
  move(new_x, new_y);
}

void View::maximize()
{
  if (maximized()) {
    return;
  }

  wlr_output* wlr_output = wlr_output_layout_output_at(layout_,
    cursor_->x(), cursor_->y());

  if (wlr_output == nullptr) {
    spdlog::warn("Failed to get output for maximize");
    return;
  }

  save_geometry();

  bool is_tiled = tiled();
  if (is_tiled) {
    set_tiled(WLR_EDGE_NONE);
  }

  set_maximized(true);

  Output *output = static_cast<Output*>(wlr_output->data);
  output->maximize_view(this);

  state = WM_WINDOW_STATE_MAXIMIZED;
}

void View::windowize()
{
  if (windowed()) {
    return;
  }

  wlr_output *wlr_output = wlr_output_layout_output_at(layout_, cursor_->x(), cursor_->y());

  if (wlr_output == nullptr) {
    spdlog::warn("Failed to get output for windowize");
    return;
  }

  if (tiled()) {
    set_tiled(WLR_EDGE_NONE);
  } else if (maximized()) {
    set_maximized(false);
  }

  set_size(saved_state_.width, saved_state_.height);

  Output *output = static_cast<Output*>(wlr_output->data);
  output->move_view(this, saved_state_.x, saved_state_.y);

  state = WM_WINDOW_STATE_WINDOW;
}

void View::tile(int edges)
{
  if (tiled()) {
    return;
  }

  save_geometry();

  bool is_maximized = maximized();
  if (is_maximized) {
    set_maximized(false);
  }

  set_tiled(edges);
  state = WM_WINDOW_STATE_TILED;
}

void View::grab()
{
  if (windowed()) {
    return;
  }

  if (tiled()) {
    set_tiled(WLR_EDGE_NONE);
  }

  if (maximized()) {
    set_maximized(false);
  }

  wlr_box box;
  geometry(&box);

  resize(saved_state_.width, saved_state_.height);

  float surface_x = cursor_->x() - x;
  float x_percentage = surface_x / box.width;
  float desired_x = saved_state_.width * x_percentage;
  x = cursor_->x() - desired_x;

  state = WM_WINDOW_STATE_WINDOW;
}

}  // namespace lumin
