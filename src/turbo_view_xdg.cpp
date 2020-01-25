#include "turbo_view_xdg.h"

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

#include "turbo_server.h"

void turbo_view_xdg::focus() {
  focus_view(xdg_surface->surface);
}

void turbo_view_xdg::geometry(struct wlr_box *box) const {
  wlr_surface_get_extends(xdg_surface->surface, box);
	/* The client never set the geometry */
	if (!xdg_surface->geometry.width) {
		return;
	}

	wlr_box_intersection(&xdg_surface->geometry, box, box);
}

const wlr_surface* turbo_view_xdg::surface() const {
  return xdg_surface->surface;
}

void turbo_view_xdg::toggle_maximize() {
  if (!maximized) {
    wlr_output* output = wlr_output_layout_output_at(server->output_layout,
      server->cursor->x, server->cursor->y);
    wlr_box *output_box = wlr_output_layout_get_box(server->output_layout, output);

    old_width = xdg_surface->geometry.width;
    old_height = xdg_surface->geometry.height;
    old_x = x;
    old_y = y;
    x = output_box->x;
    y = output_box->y;
    wlr_xdg_toplevel_set_maximized(xdg_surface, true);
    wlr_xdg_toplevel_set_size(xdg_surface, output_box->width, output_box->height);
    maximized = true;
  } else {
    x = old_x;
    y = old_y;
    wlr_xdg_toplevel_set_maximized(xdg_surface, false);
    wlr_xdg_toplevel_set_size(xdg_surface, old_width, old_height);
    maximized = false;
  }
}

void turbo_view_xdg::activate() {
  wlr_xdg_toplevel_set_activated(xdg_surface, true);
}

void turbo_view_xdg::notify_keyboard_enter() {
  wlr_keyboard *keyboard = wlr_seat_get_keyboard(server->seat);
  wlr_seat_keyboard_notify_enter(server->seat, xdg_surface->surface,
    keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
}

void turbo_view_xdg::set_size(int width, int height) {
  wlr_xdg_toplevel_set_size(xdg_surface, width, height);
}

wlr_surface* turbo_view_xdg::surface_at(double sx, double sy, double *sub_x, double *sub_y) {
  return wlr_xdg_surface_surface_at(xdg_surface, sx, sy, sub_x, sub_y);
}
