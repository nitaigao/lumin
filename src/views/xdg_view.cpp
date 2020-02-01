#include "views/xdg_view.h"

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
  #include <wlr/types/wlr_output_damage.h>
  #include <wlr/util/region.h>
  #include <wlr/util/log.h>
  #include <xkbcommon/xkbcommon.h>
}

#include "controller.h"
#include "output.h"

void XdgView::committed() {
  server->damage_output(this);
}

XdgView::XdgView(Controller* server, wlr_xdg_surface *surface)
  : View(server)
  , xdg_surface(surface)
  {}

void XdgView::scale_coords(double inx, double iny, double *outx, double *outy) const {
  *outx = inx;
  *outy = iny;
}

float XdgView::scale_output(wlr_output *output) const {
  return output->scale;
}

void XdgView::for_each_surface(wlr_surface_iterator_func_t iterator, void *data) const {
  if (xdg_surface->surface == NULL) {
    return;
  }
  wlr_xdg_surface_for_each_surface(xdg_surface, iterator, data);
}

void XdgView::focus() {
  focus_view(xdg_surface->surface);
}

void XdgView::unfocus() {
  wlr_xdg_toplevel_set_activated(xdg_surface, false);
}

void XdgView::geometry(struct wlr_box *box) const {
  wlr_surface_get_extends(xdg_surface->surface, box);

  if (!xdg_surface->geometry.width) {
    return;
  }

  wlr_box_intersection(&xdg_surface->geometry, box, box);
}

const wlr_surface* XdgView::surface() const {
  return xdg_surface->surface;
}

void XdgView::save_geometry() {
  if (state == WM_WINDOW_STATE_WINDOW) {
    wlr_box geometry;
    extents(&geometry);

    if (geometry.width != 0) {
      old_width = geometry.width;
      old_height = geometry.height;
    }

    old_x = x;
    old_y = y;
  }
}

void XdgView::tile(int edges) {
  save_geometry();

  bool is_maximized = maximized();
  if (is_maximized) {
    wlr_xdg_toplevel_set_maximized(xdg_surface, false);
  }

  wlr_xdg_toplevel_set_tiled(xdg_surface, edges);
  state = WM_WINDOW_STATE_TILED;
}

void XdgView::maximize() {
  save_geometry();

  bool is_tiled = tiled();
  if (is_tiled) {
    wlr_xdg_toplevel_set_tiled(xdg_surface, WLR_EDGE_NONE);
  }

  wlr_output* output = wlr_output_layout_output_at(server->output_layout,
    server->cursor->x, server->cursor->y);
  wlr_box *output_box = wlr_output_layout_get_box(server->output_layout, output);

  x = output_box->x;
  y = output_box->y;

  wlr_xdg_toplevel_set_maximized(xdg_surface, true);
  resize(output_box->width, output_box->height);

  state = WM_WINDOW_STATE_MAXIMIZED;
}

void XdgView::window(bool restore_position) {
  if (tiled()) {
    wlr_xdg_toplevel_set_tiled(xdg_surface, WLR_EDGE_NONE);
  }

  if (maximized()) {
    wlr_xdg_toplevel_set_maximized(xdg_surface, false);
  }

  wlr_xdg_toplevel_set_size(xdg_surface, old_width, old_height);

  View::window(restore_position);
}

void XdgView::extents(wlr_box *box) {
  wlr_xdg_surface_get_geometry(xdg_surface, box);
}

void XdgView::activate() {
  wlr_xdg_toplevel_set_activated(xdg_surface, true);
}

void XdgView::notify_keyboard_enter() {
  wlr_keyboard *keyboard = wlr_seat_get_keyboard(server->seat);
  wlr_seat_keyboard_notify_enter(server->seat, xdg_surface->surface,
    keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
}

void XdgView::resize(int width, int height) {
  wlr_xdg_toplevel_set_size(xdg_surface, width, height);
}

wlr_surface* XdgView::surface_at(double sx, double sy, double *sub_x, double *sub_y) {
  return wlr_xdg_surface_surface_at(xdg_surface, sx, sy, sub_x, sub_y);
}

View* XdgView::parent() const {
  return server->view_from_surface(xdg_surface->toplevel->parent->surface);
}

bool XdgView::is_child() const {
  bool is_child = xdg_surface->toplevel->parent != NULL;
  return is_child;
}
