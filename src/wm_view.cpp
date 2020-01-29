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

wm_view::wm_view(wm_server *server_)
  : mapped(false)
  , x(0)
  , y(0)
  , state(WM_WINDOW_STATE_WINDOW)
  , old_width(0)
  , old_height(0)
  , old_x(0)
  , old_y(0)
  , server(server_) { }

void wm_view::focus_view(wlr_surface *surface) {
  wlr_seat *seat = server->seat;
  wlr_surface *prev_surface = seat->keyboard_state.focused_surface;

  if (prev_surface == surface) {
    return;
  }

  if (prev_surface) {
    wm_view *view = server->view_from_surface(seat->keyboard_state.focused_surface);
    view->unfocus();
  }

  wl_list_remove(&link);
  wl_list_insert(&server->views, &link);

  activate();
  notify_keyboard_enter();
}

void wm_view::toggle_maximized() {
  if (state != WM_WINDOW_STATE_WINDOW) {
    windowify(true);
  } else {
    maximize();
  }
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

void wm_view::tile_left() {
  wlr_box box;
  extends(&box);

  int corner_x = x + box.x + (box.width / 2.0f);
  int corner_y = y + box.y + (box.height / 2.0f);
  wlr_output* output = wlr_output_layout_output_at(server->output_layout, corner_x, corner_y);

  int width = (output->width / 2.0f) / output->scale;
  int height = output->height / output->scale;
  set_size(width, height);

  x = 0;
  y = 0;

  wlr_output_layout_output_coords(server->output_layout, output, &x, &y);

  tile(WLR_EDGE_LEFT);
}

void wm_view::tile_right() {
  wlr_box box;
  extends(&box);

  int corner_x = x + box.x + (box.width / 2.0f);
  int corner_y = y + box.y + (box.height / 2.0f);
  wlr_output* output = wlr_output_layout_output_at(server->output_layout, corner_x, corner_y);

  y = 0;
  x = 0;

  wlr_output_layout_output_coords(server->output_layout, output, &x, &y);

  // middle of the screen
  x += (output->width / 2.0f) / output->scale;

  int width = (output->width / 2.0f) / output->scale;
  int height = output->height / output->scale;

  set_size(width, height);
  tile(WLR_EDGE_RIGHT);
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

  server->grab_x = 0;
  server->grab_y = 0;

  server->grabbed_view = this;
  server->cursor_mode = mode;

  wlr_box geo_box;
  extends(&geo_box);

  // scale_coords(server->cursor->x, server->cursor->y, &server->grab_x, &server->grab_y);

  if (mode == WM_CURSOR_MOVE) {
    server->grab_x = server->cursor->x - x;
    server->grab_y = server->cursor->y - y;
  } else {
    server->grab_x = x;
    server->grab_y = y;
    server->grab_cursor_x = server->cursor->x;
    server->grab_cursor_y = server->cursor->y;
  }

  server->grab_width = geo_box.width;
  server->grab_height = geo_box.height;

  server->resize_edges = edges;
}

void wm_view::map_view() {
  mapped = true;
  server->position_view(this);
  focus();
}

void wm_view::unmap_view() {
  mapped = false;
  server->pop_view(this);
}

