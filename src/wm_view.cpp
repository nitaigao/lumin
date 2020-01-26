#include "wm_view.h"

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
#include "wm_cursor_mode.h"

wm_view::wm_view()
  : mapped(false)
  , x(0)
  , y(0)
  , maximized(false)
  , old_width(0)
  , old_height(0)
  , old_x(0)
  , old_y(0) { }

void wm_view::focus_view(wlr_surface *surface) {
  /* Note: this function only deals with keyboard focus. */
  wlr_seat *seat = server->seat;
  wlr_surface *prev_surface = seat->keyboard_state.focused_surface;
  if (prev_surface == surface) {
    /* Don't re-focus an already focused surface. */
    return;
  }

  if (prev_surface) {
    if (wlr_surface_is_xdg_surface(seat->keyboard_state.focused_surface)) {
      /*
      * Deactivate the previously focused surface. This lets the client know
      * it no longer has focus and the client will repaint accordingly, e.g.
      * stop displaying a caret.
      */
      struct wlr_xdg_surface *previous = wlr_xdg_surface_from_wlr_surface(seat->keyboard_state.focused_surface);
      wlr_xdg_toplevel_set_activated(previous, false);
    }

    if (wlr_surface_is_xwayland_surface(seat->keyboard_state.focused_surface)) {
      struct wlr_xwayland_surface *previous = wlr_xwayland_surface_from_wlr_surface(seat->keyboard_state.focused_surface);
      wlr_xwayland_surface_activate(previous, false);
    }
  }

  /* Move the view to the front */
  wl_list_remove(&link);
  wl_list_insert(&server->views, &link);

  activate();
  notify_keyboard_enter();
}

bool wm_view::view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy) {
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

void wm_view::begin_interactive(enum wm_cursor_mode mode, uint32_t edges) {
  /* This function sets up an interactive move or resize operation, where the
   * compositor stops propegating pointer events to clients and instead
   * consumes them itself, to move or resize windows. */
  wlr_surface *focused_surface = server->seat->pointer_state.focused_surface;

  if (surface() != focused_surface) {
    /* Deny move/resize requests from unfocused clients. */
    return;
  }

  server->grabbed_view = this;
  server->cursor_mode = mode;

  wlr_box geo_box;
  geometry(&geo_box);

  scale_coords(server->cursor->x, server->cursor->y, &server->grab_x, &server->grab_y);

  if (mode == wm_CURSOR_MOVE) {
    server->grab_x -= x;
    server->grab_y -= y;
  } else {
    server->grab_x += geo_box.x;
    server->grab_y += geo_box.y;
  }

  server->grab_width = geo_box.width;
  server->grab_height = geo_box.height;
  server->resize_edges = edges;
}