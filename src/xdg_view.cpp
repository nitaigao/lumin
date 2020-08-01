#include "xdg_view.h"

#include <wlroots.h>
#include <spdlog/spdlog.h>

#include "cursor_mode.h"
#include "cursor.h"
#include "output.h"
#include "seat.h"

namespace lumin {

XDGView::XDGView(wlr_xdg_surface *surface, Cursor *cursor, wlr_output_layout *layout, Seat *seat)
  : View(cursor, layout, seat)
  , xdg_surface_(surface)
{
  map.notify = XDGView::xdg_surface_map_notify;
  wl_signal_add(&xdg_surface_->events.map, &map);

  unmap.notify = XDGView::xdg_surface_unmap_notify;
  wl_signal_add(&xdg_surface_->events.unmap, &unmap);

  destroy.notify = XDGView::xdg_surface_destroy_notify;
  wl_signal_add(&xdg_surface_->events.destroy, &destroy);

  commit.notify = XDGView::xdg_surface_commit_notify;
  wl_signal_add(&xdg_surface_->surface->events.commit, &commit);

  new_subsurface.notify = XDGView::new_subsurface_notify;
  wl_signal_add(&xdg_surface_->surface->events.new_subsurface, &new_subsurface);

  new_popup.notify = XDGView::new_popup_notify;
  wl_signal_add(&xdg_surface_->events.new_popup, &new_popup);

  struct wlr_xdg_toplevel *toplevel = xdg_surface_->toplevel;

  request_move.notify = XDGView::xdg_toplevel_request_move_notify;
  wl_signal_add(&toplevel->events.request_move, &request_move);

  request_resize.notify = XDGView::xdg_toplevel_request_resize_notify;
  wl_signal_add(&toplevel->events.request_resize, &request_resize);

  request_maximize.notify = XDGView::xdg_toplevel_request_maximize_notify;
  wl_signal_add(&toplevel->events.request_maximize, &request_maximize);

  request_minimize.notify = XDGView::xdg_toplevel_request_minimize_notify;
  wl_signal_add(&toplevel->events.request_minimize, &request_minimize);

  request_fullscreen.notify = XDGView::xdg_toplevel_request_fullscreen_notify;
  wl_signal_add(&toplevel->events.request_fullscreen, &request_fullscreen);

  surface->data = this;
}

std::string XDGView::id() const
{
  if (!mapped) {
    return "";
  }

  if (xdg_surface_->toplevel->app_id == nullptr) {
    return "";
  }

  if (strlen(xdg_surface_->toplevel->app_id) == 0) {
    return "";
  }

  return xdg_surface_->toplevel->app_id;
}

std::string XDGView::title() const
{
  return xdg_surface_->toplevel->title;
}

uint XDGView::min_width() const
{
  return xdg_surface_->toplevel->current.min_width;
}

uint XDGView::min_height() const
{
  return xdg_surface_->toplevel->current.min_height;
}

void surface_send_enter(wlr_surface *surface, int sx, int sy, void *data)
{
  auto output = static_cast<wlr_output*>(data);
  wlr_surface_send_enter(surface, output);
}

void XDGView::enter(const Output* output)
{
  for_each_surface(surface_send_enter, output->wlr_output);
}

void XDGView::set_tiled(int edges)
{
  wlr_xdg_toplevel_set_maximized(xdg_surface_, edges != WLR_EDGE_NONE);
}

void XDGView::set_maximized(bool maximized)
{
  wlr_xdg_toplevel_set_maximized(xdg_surface_, maximized);
}

void XDGView::for_each_surface(wlr_surface_iterator_func_t iterator, void *data) const
{
  if (xdg_surface_->surface == NULL) {
    return;
  }
  wlr_xdg_surface_for_each_surface(xdg_surface_, iterator, data);
}

wlr_surface* XDGView::surface() const
{
  return xdg_surface_->surface;
}

bool XDGView::can_move() const
{
  return !is_menubar() && !is_shell();
}

void XDGView::extents(struct wlr_box *box) const
{
  wlr_surface_get_extends(xdg_surface_->surface, box);

  if (!xdg_surface_->geometry.width) {
    return;
  }

  wlr_box_intersection(&xdg_surface_->geometry, box, box);
}

bool XDGView::has_surface(const wlr_surface *surface) const
{
  return xdg_surface_->surface == surface;
}

void XDGView::set_size(int width, int height)
{
  wlr_xdg_toplevel_set_size(xdg_surface_, width, height);
}

void XDGView::geometry(wlr_box *box) const {
  wlr_xdg_surface_get_geometry(xdg_surface_, box);
}

void XDGView::activate()
{
  wlr_xdg_toplevel_set_activated(xdg_surface_, true);
}

void XDGView::deactivate()
{
  wlr_xdg_toplevel_set_activated(xdg_surface_, false);
}

void XDGView::move(int new_x, int new_y)
{
  x = new_x;
  y = new_y;

  on_move.emit(this);
}

void XDGView::resize(double width, double height)
{
  wlr_xdg_toplevel_set_size(xdg_surface_, width, height);
}

wlr_surface* XDGView::surface_at(double sx, double sy, double *sub_x, double *sub_y) {
  return wlr_xdg_surface_surface_at(xdg_surface_, sx, sy, sub_x, sub_y);
}

View* XDGView::parent() const
{
  auto parent_view = static_cast<View*>(xdg_surface_->toplevel->parent->surface->data);
  return parent_view;
}

bool XDGView::is_root() const
{
  return !is_child();
}

bool XDGView::is_child() const
{
  bool is_child = xdg_surface_->toplevel->parent != NULL;
  return is_child;
}

const View* XDGView::root() const
{
  if (xdg_surface_->toplevel->parent == NULL) {
    return this;
  }
  return parent()->root();
}

void XDGView::xdg_toplevel_request_move_notify(wl_listener *listener, void *data)
{
  XDGView *view = wl_container_of(listener, view, request_move);
  if (!view->can_move()) {
    return;
  }
  view->grab();
  view->cursor_->begin_interactive(view, WM_CURSOR_MOVE, WLR_EDGE_NONE);
}

void XDGView::xdg_toplevel_request_resize_notify(wl_listener *listener, void *data)
{
  auto event = static_cast<wlr_xdg_toplevel_resize_event*>(data);
  XDGView *view = wl_container_of(listener, view, request_resize);
  view->cursor_->begin_interactive(view, WM_CURSOR_RESIZE, event->edges);
}

void XDGView::xdg_toplevel_request_maximize_notify(wl_listener *listener, void *data)
{
  XDGView *view = wl_container_of(listener, view, request_maximize);
  view->toggle_maximized();
}

void XDGView::xdg_toplevel_request_minimize_notify(wl_listener *listener, void *data)
{
  XDGView *view = wl_container_of(listener, view, request_minimize);
  view->minimize();
}

void XDGView::xdg_toplevel_request_fullscreen_notify(wl_listener *listener, void *data)
{
  auto event = static_cast<wlr_xdg_toplevel_set_fullscreen_event*>(data);
  XDGView *view = wl_container_of(listener, view, request_fullscreen);
  view->state = event->fullscreen ? WM_WINDOW_STATE_FULLSCREEN : WM_WINDOW_STATE_WINDOW;
}

void XDGView::xdg_surface_destroy_notify(wl_listener *listener, void *data)
{
  XDGView *view = wl_container_of(listener, view, destroy);
  view->on_destroy.emit(view);
}

void XDGView::xdg_popup_subsurface_commit_notify(wl_listener *listener, void *data)
{
  Subsurface *subsurface = wl_container_of(listener, subsurface, commit);
  subsurface->view->on_damage.emit(subsurface->view);
}

void XDGView::xdg_subsurface_commit_notify(wl_listener *listener, void *data)
{
  Subsurface *subsurface = wl_container_of(listener, subsurface, commit);
  subsurface->view->on_damage.emit(subsurface->view);
}

void XDGView::xdg_popup_destroy_notify(wl_listener *listener, void *data)
{
  Popup *popup = wl_container_of(listener, popup, destroy);
  popup->view->on_damage.emit(popup->view);
}

void XDGView::xdg_popup_commit_notify(wl_listener *listener, void *data)
{
  Popup *popup = wl_container_of(listener, popup, commit);
  popup->view->on_damage.emit(popup->view);
}

void XDGView::xdg_surface_commit_notify(wl_listener *listener, void *data)
{
  XDGView *view = wl_container_of(listener, view, commit);
  if (view->mapped) {
    view->on_damage.emit(view);
  }
}

void XDGView::new_popup_subsurface_notify(wl_listener *listener, void *data)
{
  auto *wlr_subsurface_ = static_cast<wlr_subsurface*>(data);
  Popup *popup = wl_container_of(listener, popup, new_subsurface);

  auto subsurface = new Subsurface();
  subsurface->view = popup->view;

  subsurface->commit.notify = xdg_popup_subsurface_commit_notify;
  wl_signal_add(&wlr_subsurface_->surface->events.commit, &subsurface->commit);
}

void XDGView::new_subsurface_notify(wl_listener *listener, void *data)
{
  auto *wlr_subsurface_ = static_cast<wlr_subsurface*>(data);
  XDGView *view = wl_container_of(listener, view, new_subsurface);

  auto subsurface = new Subsurface();
  subsurface->view = view;

  subsurface->commit.notify = xdg_subsurface_commit_notify;
  wl_signal_add(&wlr_subsurface_->surface->events.commit, &subsurface->commit);
}

void XDGView::new_popup_popup_notify(wl_listener *listener, void *data)
{
  auto *xdg_popup = static_cast<wlr_xdg_popup*>(data);
  Popup *parent_popup = wl_container_of(listener, parent_popup, new_popup);

  auto popup = new Popup();
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

void XDGView::new_popup_notify(wl_listener *listener, void *data) {
  auto *xdg_popup = static_cast<wlr_xdg_popup*>(data);

  XDGView *view = wl_container_of(listener, view, new_popup);

  auto popup = new Popup();
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

void XDGView::xdg_surface_map_notify(wl_listener *listener, void *data)
{
  XDGView *view = wl_container_of(listener, view, map);
  view->mapped = true;
  view->on_map.emit(view);
}

void XDGView::xdg_surface_unmap_notify(wl_listener *listener, void *data)
{
  XDGView *view = wl_container_of(listener, view, unmap);
  view->mapped = false;
  view->on_unmap.emit(view);
}

}  // namespace lumin
