#include "wm_view_xwayland.h"

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

#include "wm_server.h"
#include "wm_output.h"

wm_view_xwayland::wm_view_xwayland()
  : wm_view() {

  }

void wm_view_xwayland::scale_coords(double inx, double iny, double *outx, double *outy) const {
  wlr_output* output = wlr_output_layout_output_at(server->output_layout, inx, iny);
  *outx = inx * output->scale;
  *outy = iny * output->scale;
}

void wm_view_xwayland::for_each_surface(wlr_surface_iterator_func_t iterator, void *data) const {
  if (xwayland_surface->surface == NULL) {
    return;
  }
  wlr_surface_for_each_surface(xwayland_surface->surface, iterator, data);
}

const wlr_surface* wm_view_xwayland::surface() const {
  return xwayland_surface->surface;
}

void wm_view_xwayland::focus() {
  focus_view(xwayland_surface->surface);
}

float wm_view_xwayland::scale_output(wlr_output *output) const {
  return 1.0f;
}

void wm_view_xwayland::toggle_maximize() {
  if (!maximized) {
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
    wlr_xwayland_surface_configure(xwayland_surface, output_box->x, output_box->y,
      output_box->width * output->scale, output_box->height * output->scale);
    maximized = true;
  } else {
    wlr_xwayland_surface_set_maximized(xwayland_surface, false);
    wlr_xwayland_surface_configure(xwayland_surface, old_x, old_y, old_width, old_height);
    x = old_x;
    y = old_y;
    maximized = false;
  }
}

wlr_surface* wm_view_xwayland::surface_at(double sx, double sy, double *sub_x, double *sub_y) {
  if (xwayland_surface->surface == NULL) {
    return NULL;
  }
  return wlr_surface_surface_at(xwayland_surface->surface, sx, sy, sub_x, sub_y);
}

void wm_view_xwayland::activate() {
  wlr_xwayland_surface_activate(xwayland_surface, true);
}

void wm_view_xwayland::notify_keyboard_enter() {
  wlr_keyboard *keyboard = wlr_seat_get_keyboard(server->seat);
  /*
   * Tell the seat to have the keyboard enter this surface. wlroots will keep
   * track of this and automatically send key events to the appropriate
   * clients without additional work on your part.
   */
  wlr_seat_keyboard_notify_enter(server->seat, xwayland_surface->surface,
    keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
}


void wm_view_xwayland::set_size(int width, int height) {
  wlr_xwayland_surface_configure(xwayland_surface, x, y, width, height);
}


void wm_view_xwayland::geometry(struct wlr_box *box) const {
  wlr_surface_get_extends(xwayland_surface->surface, box);
	/* The client never set the geometry */
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

wm_view* wm_view_xwayland::parent() const {
  return server->view_from_surface(xwayland_surface->parent->surface);
}

bool wm_view_xwayland::is_child() const {
  bool is_child = xwayland_surface->parent != NULL;
  return is_child;
}

