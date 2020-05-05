#include "xwayland_view.h"

#include <wlroots.h>
#include <spdlog/spdlog.h>

#include "cursor_mode.h"
#include "cursor.h"
#include "output.h"
#include "seat.h"

const int DEFAULT_MINIMUM_WIDTH = 800;
const int DEFAULT_MINIMUM_HEIGHT = 600;

namespace lumin {

XWaylandView::XWaylandView(
  wlr_xwayland_surface *surface,
  Cursor *cursor,
  wlr_output_layout *layout,
  Seat *seat)
  : View(cursor, layout, seat)
  , xwayland_surface_(surface)
{
  map.notify = XWaylandView::xwayland_surface_map_notify;
  wl_signal_add(&xwayland_surface_->events.map, &map);

  unmap.notify = XWaylandView::xwayland_surface_unmap_notify;
  wl_signal_add(&xwayland_surface_->events.unmap, &unmap);

  request_move.notify = XWaylandView::xwayland_request_move_notify;
  wl_signal_add(&xwayland_surface_->events.request_move, &request_move);

  request_resize.notify = XWaylandView::xwayland_request_resize_notify;
  wl_signal_add(&xwayland_surface_->events.request_resize, &request_resize);

  request_configure.notify = XWaylandView::xwayland_request_configure_notify;
  wl_signal_add(&xwayland_surface_->events.request_configure, &request_configure);

  request_maximize.notify = XWaylandView::xwayland_request_maximize_notify;
  wl_signal_add(&xwayland_surface_->events.request_maximize, &request_maximize);
}

void XWaylandView::geometry(wlr_box *box) const
{
  box->x = xwayland_surface_->x;
  box->y = xwayland_surface_->y;
  box->width = xwayland_surface_->width;
  box->height = xwayland_surface_->height;
}

void XWaylandView::extents(wlr_box *box) const
{
  wlr_surface_get_extends(xwayland_surface_->surface, box);
}

wlr_surface* XWaylandView::surface() const
{
  return xwayland_surface_->surface;
}

void XWaylandView::resize(double width, double height)
{
  wlr_xwayland_surface_configure(xwayland_surface_, x, y, width, height);
}

void XWaylandView::activate()
{
  wlr_xwayland_surface_activate(xwayland_surface_, true);
}

void XWaylandView::deactivate()
{
  wlr_xwayland_surface_activate(xwayland_surface_, false);
}

void XWaylandView::set_size(int width, int height)
{
  wlr_xwayland_surface_configure(xwayland_surface_, x, y, width, height);
}

void XWaylandView::move(int new_x, int new_y) {
  x = new_x;
  y = new_y;

  on_move.emit(this);
}

void XWaylandView::set_tiled(int edges)
{
  wlr_xwayland_surface_set_maximized(xwayland_surface_, edges);
}

void XWaylandView::set_maximized(bool maximized)
{
  wlr_xwayland_surface_set_maximized(xwayland_surface_, maximized);
}

void XWaylandView::focus()
{
  wlr_xwayland_surface_activate(xwayland_surface_, true);
}

void XWaylandView::unfocus()
{
  wlr_xwayland_surface_activate(xwayland_surface_, false);
}

wlr_surface* XWaylandView::surface_at(double sx, double sy, double *sub_x, double *sub_y)
{
  return wlr_surface_surface_at(xwayland_surface_->surface, sx, sy, sub_x, sub_y);
}

void XWaylandView::tile(int edges) {
  if (tiled()) {
    return;
  }

  save_geometry();

  bool is_maximized = maximized();
  if (is_maximized) {
    wlr_xwayland_surface_set_maximized(xwayland_surface_, true);
  }

  state = WM_WINDOW_STATE_TILED;
}

std::string XWaylandView::id() const
{
  std::string id(xwayland_surface_->class_name);

  if (id.empty()) {
    return "";
  }

  return id;
}

std::string XWaylandView::title() const
{
  return xwayland_surface_->title;
}

void XWaylandView::enter(const Output* output)
{
  wlr_surface_send_enter(xwayland_surface_->surface, output->wlr_output);
}

uint XWaylandView::min_width() const
{
  return DEFAULT_MINIMUM_WIDTH;
}

uint XWaylandView::min_height() const
{
  return DEFAULT_MINIMUM_HEIGHT;
}

bool XWaylandView::is_root() const
{
  return true;
}

View* XWaylandView::parent() const
{
  return NULL;
}

const View *XWaylandView::root() const
{
  return this;
}

bool XWaylandView::has_surface(const wlr_surface *surface) const
{
  return xwayland_surface_->surface == surface;
}

void XWaylandView::for_each_surface(wlr_surface_iterator_func_t iterator, void *data) const
{
  wlr_surface_for_each_surface(xwayland_surface_->surface, iterator, data);
}

void XWaylandView::xwayland_surface_commit_notify(wl_listener *listener, void *data)
{
  XWaylandView *view = wl_container_of(listener, view, commit);
  view->on_commit.emit(view);
}

void XWaylandView::xwayland_surface_map_notify(wl_listener *listener, void *data)
{
  XWaylandView *view = wl_container_of(listener, view, map);

  view->commit.notify = XWaylandView::xwayland_surface_commit_notify;
  wl_signal_add(&view->xwayland_surface_->surface->events.commit, &view->commit);

  view->mapped = true;
  view->on_map.emit(view);
}

void XWaylandView::xwayland_surface_unmap_notify(wl_listener *listener, void *data)
{
  XWaylandView *view = wl_container_of(listener, view, unmap);
  view->mapped = false;
  view->on_unmap.emit(view);
}

void XWaylandView::xwayland_request_move_notify(wl_listener *listener, void *data)
{
  XWaylandView *view = wl_container_of(listener, view, request_move);
  view->grab();
  view->cursor_->begin_interactive(view, WM_CURSOR_MOVE, WLR_EDGE_NONE);
}

void XWaylandView::xwayland_request_resize_notify(wl_listener *listener, void *data)
{
  auto event = static_cast<wlr_xwayland_resize_event*>(data);
  XWaylandView *view = wl_container_of(listener, view, request_resize);
  view->cursor_->begin_interactive(view, WM_CURSOR_RESIZE, event->edges);
}

void XWaylandView::xwayland_request_configure_notify(wl_listener *listener, void *data)
{
  XWaylandView *view = wl_container_of(listener, view, request_configure);
  auto event = static_cast<struct wlr_xwayland_surface_configure_event *>(data);
  wlr_xwayland_surface_configure(view->xwayland_surface_,
    event->x, event->y, event->width, event->height);
}

void XWaylandView::xwayland_request_maximize_notify(wl_listener *listener, void *data)
{
  XWaylandView *view = wl_container_of(listener, view, request_maximize);
  view->toggle_maximized();
}

}  // namespace lumin
