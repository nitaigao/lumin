#include "turbo_view.h"

extern "C" {
  #include <unistd.h>
  #include <wayland-server-core.h>
  #include <wlr/backend.h>
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
#include "turbo_cursor_mode.h"

void turbo_view::toggle_maximize() {
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
    wlr_xdg_toplevel_set_maximized(xdg_surface, false);
    wlr_xdg_toplevel_set_size(xdg_surface, old_width, old_height);
    x = old_x;
    y = old_y;
    maximized = false;
  }
}

void turbo_view::focus_view(wlr_surface *surface) {
  /* Note: this function only deals with keyboard focus. */
  wlr_seat *seat = server->seat;
  wlr_surface *prev_surface = seat->keyboard_state.focused_surface;
  if (prev_surface == surface) {
    /* Don't re-focus an already focused surface. */
    return;
  }

  if (prev_surface) {
    /*
     * Deactivate the previously focused surface. This lets the client know
     * it no longer has focus and the client will repaint accordingly, e.g.
     * stop displaying a caret.
     */
    struct wlr_xdg_surface *previous = wlr_xdg_surface_from_wlr_surface(
          seat->keyboard_state.focused_surface);
    wlr_xdg_toplevel_set_activated(previous, false);
  }

  wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);
  /* Move the view to the front */
  wl_list_remove(&link);
  wl_list_insert(&server->views, &link);
  /* Activate the new surface */
  wlr_xdg_toplevel_set_activated(xdg_surface, true);
  /*
   * Tell the seat to have the keyboard enter this surface. wlroots will keep
   * track of this and automatically send key events to the appropriate
   * clients without additional work on your part.
   */
  wlr_seat_keyboard_notify_enter(seat, xdg_surface->surface,
    keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
}

bool turbo_view::view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy) {
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
  wlr_surface *_surface = NULL;
  _surface = wlr_xdg_surface_surface_at(xdg_surface, view_sx, view_sy, &_sx, &_sy);

  if (_surface != NULL) {
    *sx = _sx;
    *sy = _sy;
    *surface = _surface;
    return true;
  }

  return false;
}

void turbo_view::begin_interactive(enum turbo_cursor_mode mode, uint32_t edges) {
  /* This function sets up an interactive move or resize operation, where the
   * compositor stops propegating pointer events to clients and instead
   * consumes them itself, to move or resize windows. */
  wlr_surface *focused_surface = server->seat->pointer_state.focused_surface;

  if (xdg_surface->surface != focused_surface) {
    /* Deny move/resize requests from unfocused clients. */
    return;
  }

  server->grabbed_view = this;
  server->cursor_mode = mode;

  struct wlr_box geo_box;
  wlr_xdg_surface_get_geometry(xdg_surface, &geo_box);

  if (mode == TURBO_CURSOR_MOVE) {
    server->grab_x = server->cursor->x - x;
    server->grab_y = server->cursor->y - y;
  } else {
    server->grab_x = server->cursor->x + geo_box.x;
    server->grab_y = server->cursor->y + geo_box.y;
  }

  server->grab_width = geo_box.width;
  server->grab_height = geo_box.height;
  server->resize_edges = edges;
}
