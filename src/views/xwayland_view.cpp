#include "views/xwayland_view.h"

extern "C" {
  #include <unistd.h>
  #include <wayland-server-core.h>
  #include <wlr/backend.h>
  #include <wlr/xwayland.h>
  #include <wlr/render/wlr_renderer.h>
  #include <wlr/types/wlr_cursor.h>
  #include <wlr/types/wlr_compositor.h>
  #include <wlr/types/wlr_data_device.h>
  #include <wlr/types/wlr_input_device.h>
  #include <wlr/types/wlr_keyboard.h>
  #include <wlr/types/wlr_matrix.h>
  #include <wlr/types/wlr_output.h>
  #include <wlr/types/wlr_output_layout.h>
  #include <wlr/types/wlr_pointer.h>
  #include <wlr/types/wlr_seat.h>
  #include <wlr/types/wlr_xcursor_manager.h>
  #include <wlr/types/wlr_xdg_shell.h>
  #include <wlr/util/log.h>
  #include <xkbcommon/xkbcommon.h>
}

#include "controller.h"
#include "output.h"

void XWaylandView::save_geometry() {
  if (state == WM_WINDOW_STATE_WINDOW) {
    old_width = xwayland_surface->width;
    old_height = xwayland_surface->height;

    old_x = x;
    old_y = y;
  }
}

void XWaylandView::committed() { }

void XWaylandView::tile(int edges) {
  save_geometry();
  wlr_xwayland_surface_set_maximized(xwayland_surface, true);
  state = WM_WINDOW_STATE_TILED;
}

XWaylandView::XWaylandView(Controller *server, wlr_xwayland_surface *surface)
  : View(server)
  , xwayland_surface(surface)
  { }

void XWaylandView::scale_coords(double inx, double iny, double *outx, double *outy) const {
  *outx = inx;
  *outy = iny;
}

void XWaylandView::for_each_surface(wlr_surface_iterator_func_t iterator, void *data) const {
  if (xwayland_surface->surface == NULL) {
    return;
  }
  wlr_surface_for_each_surface(xwayland_surface->surface, iterator, data);
}

const wlr_surface* XWaylandView::surface() const {
  return xwayland_surface->surface;
}

void XWaylandView::focus() {
  focus_view(xwayland_surface->surface);
}

void XWaylandView::unfocus() {
  wlr_xwayland_surface_activate(xwayland_surface, false);
}

float XWaylandView::scale_output(wlr_output *output) const {
  return 1.0f;
}

void XWaylandView::extents(wlr_box *box) const {
}

void XWaylandView::maximize() {
  if (state == WM_WINDOW_STATE_MAXIMIZED) {
    return;
  }

  wlr_output* output = wlr_output_layout_output_at(server->output_layout,
    server->cursor->x, server->cursor->y);
  wlr_box *output_box = wlr_output_layout_get_box(server->output_layout, output);

  old_width = xwayland_surface->width;
  old_height = xwayland_surface->height;

  old_x = x;
  old_y = y;

  x = output_box->x;
  y = output_box->y;

  wlr_xwayland_surface_set_maximized(xwayland_surface, true);
  resize(output_box->width * output->scale, output_box->height * output->scale);

  state = WM_WINDOW_STATE_MAXIMIZED;
}

void XWaylandView::window(bool restore_position) {
  if (state == WM_WINDOW_STATE_WINDOW) {
    return;
  }
  View::window(restore_position);
  wlr_xwayland_surface_set_maximized(xwayland_surface, false);
  wlr_xwayland_surface_configure(xwayland_surface, old_x, old_y, old_width, old_height);
}

wlr_surface* XWaylandView::surface_at(double sx, double sy, double *sub_x, double *sub_y) {
  if (xwayland_surface->surface == NULL) {
    return NULL;
  }
  return wlr_surface_surface_at(xwayland_surface->surface, sx, sy, sub_x, sub_y);
}

void XWaylandView::activate() {
  wlr_xwayland_surface_activate(xwayland_surface, true);
}

void XWaylandView::notify_keyboard_enter() {
  wlr_keyboard *keyboard = wlr_seat_get_keyboard(server->seat);
  wlr_seat_keyboard_notify_enter(server->seat, xwayland_surface->surface,
    keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
}


void XWaylandView::resize(int width, int height) {
  wlr_xwayland_surface_configure(xwayland_surface, x, y, width, height);
}


void XWaylandView::geometry(struct wlr_box *box) const {
  wlr_surface_get_extends(xwayland_surface->surface, box);

  if (!xwayland_surface->width) {
    return;
  }

  wlr_box geometry;
  geometry.x = xwayland_surface->x;
  geometry.y = xwayland_surface->y;
  geometry.width = xwayland_surface->width;
  geometry.height = xwayland_surface->height;

  wlr_box_intersection(&geometry, box, box);
}

View* XWaylandView::parent() const {
  return server->view_from_surface(xwayland_surface->parent->surface);
}

bool XWaylandView::is_child() const {
  bool is_child = xwayland_surface->parent != NULL;
  return is_child;
}

