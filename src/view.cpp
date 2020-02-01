#include "view.h"

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
  #include <wlr/types/wlr_output_damage.h>
  #include <wlr/types/wlr_xcursor_manager.h>
  #include <wlr/types/wlr_xdg_shell.h>
  #include <wlr/util/log.h>
  #include <xkbcommon/xkbcommon.h>
}

#include "controller.h"
#include "output.h"
#include "cursor_mode.h"

const int DEFAULT_MINIMUM_WIDTH = 800;
const int DEFAULT_MINIMUM_HEIGHT = 600;

View::~View() {
  // wl_list_init(&map.link);
  // wl_list_remove(&map.link);

  // wl_list_init(&unmap.link);
  // wl_list_remove(&unmap.link);

  // wl_list_init(&commit.link);
  // wl_list_remove(&commit.link);

  // wl_list_init(&destroy.link);
  // wl_list_remove(&destroy.link);

  // wl_list_init(&request_move.link);
  // wl_list_remove(&request_move.link);

  // wl_list_init(&request_resize.link);
  // wl_list_remove(&request_resize.link);

  // wl_list_init(&request_maximize.link);
  // wl_list_remove(&request_maximize.link);

  // wl_list_init(&new_subsurface.link);
  // wl_list_remove(&new_subsurface.link);

  // wl_list_init(&new_popup.link);
  // wl_list_remove(&new_popup.link);
}

View::View(Controller *server_)
  : mapped(false)
  , x(0)
  , y(0)
  , state(WM_WINDOW_STATE_WINDOW)
  , old_width(DEFAULT_MINIMUM_WIDTH)
  , old_height(DEFAULT_MINIMUM_HEIGHT)
  , old_x(0)
  , old_y(0)
  , server(server_) { }

void View::focus_view(wlr_surface *surface) {
  wlr_seat *seat = server->seat;
  wlr_surface *prev_surface = seat->keyboard_state.focused_surface;

  if (prev_surface == surface) {
    return;
  }

  if (prev_surface) {
    View *view = server->view_from_surface(seat->keyboard_state.focused_surface);
    view->unfocus();
  }

  wl_list_remove(&link);
  wl_list_insert(&server->views, &link);

  activate();
  notify_keyboard_enter();
}

bool View::windowed() const {
  bool windowed = state == WM_WINDOW_STATE_WINDOW;
  return windowed;
}

bool View::tiled() const {
  bool tiled = state == WM_WINDOW_STATE_TILED;
  return tiled;
}

bool View::maximized() const {
  bool maximised = state == WM_WINDOW_STATE_MAXIMIZED;
  return maximised;
}

void View::window(bool restore_position) {
  bool is_windowed = windowed();
  if (is_windowed) {
    return;
  }

  if (restore_position) {
    x = old_x;
    y = old_y;
  } else {
    wlr_box geometry;
    extents(&geometry);

    float surface_x = server->cursor->x - x;
    float x_percentage = surface_x / geometry.width;
    float desired_x = old_width * x_percentage;
    x = server->cursor->x - desired_x;
  }

  server->damage_outputs();
  state = WM_WINDOW_STATE_WINDOW;
}

void View::toggle_maximized() {
  bool is_maximised = maximized();

  if (is_maximised) {
    window(true);
  } else {
    maximize();
  }
}

bool View::view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy) {
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

void View::tile_left() {
  tile(WLR_EDGE_LEFT);

  wlr_box box;
  extents(&box);

  int corner_x = x + box.x + (box.width / 2.0f);
  int corner_y = y + box.y + (box.height / 2.0f);
  wlr_output* output = wlr_output_layout_output_at(server->output_layout, corner_x, corner_y);

  int width = (output->width / 2.0f) / output->scale;
  int height = output->height / output->scale;
  resize(width, height);

  x = 0;
  y = 0;

  wlr_output_layout_output_coords(server->output_layout, output, &x, &y);
}

void View::tile_right() {
  tile(WLR_EDGE_RIGHT);

  wlr_box box;
  extents(&box);

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

  resize(width, height);
}

void View::begin_interactive(enum CursorMode mode, uint32_t edges) {
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
  server->CursorMode = mode;

  wlr_box geo_box;
  extents(&geo_box);

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

void View::map_view() {
  mapped = true;
  server->position_view(this);
  focus();
  server->damage_outputs();
}

void View::unmap_view() {
  mapped = false;
  server->focus_top();
  server->damage_outputs();
}
