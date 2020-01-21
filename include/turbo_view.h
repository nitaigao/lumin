#ifndef TURBO_VIEW_H
#define TURBO_VIEW_H

#include <wayland-server-core.h>

#include "turbo_cursor_mode.h"

struct turbo_server;
struct wlr_xdg_surface;
struct wlr_surface;

struct turbo_view {
  wl_list link;
  turbo_server *server;
  wlr_xdg_surface *xdg_surface;
  wl_listener map;
  wl_listener unmap;
  wl_listener destroy;
  wl_listener request_move;
  wl_listener request_resize;
  wl_listener request_maximize;
  bool mapped;
  int x, y;
  bool maximized;

  int old_width, old_height;
  int old_x, old_y;

  bool view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy);

  void focus_view(wlr_surface *surface);

  void begin_interactive(enum turbo_cursor_mode mode, uint32_t edges);

  void toggle_maximize();
};


#endif
