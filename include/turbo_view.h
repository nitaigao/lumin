#ifndef TURBO_VIEW_H
#define TURBO_VIEW_H

#include <wayland-server-core.h>

#include "turbo_cursor_mode.h"

struct turbo_server;

struct wlr_xdg_surface;
struct wlr_xwayland_surface;
struct wlr_surface;
struct wlr_box;
struct wlr_output;

enum surface_type {
  TURBO_SURFACE_TYPE_NONE = 0,
  TURBO_XDG_SURFACE = 1,
  TURBO_XWAYLAND_SURFACE = 2
};

typedef void (*wlr_surface_iterator_func_t)(struct wlr_surface *surface,
	int sx, int sy, void *data);

struct turbo_view {
  virtual ~turbo_view() {};
  turbo_view();

  wl_list link;

  turbo_server *server;

  // enum surface_type surface_type;

  wl_listener map;
  wl_listener unmap;
  wl_listener destroy;
  wl_listener request_move;
  wl_listener request_resize;
  wl_listener request_maximize;

  bool mapped;
  double x, y;
  bool maximized;

  int old_width, old_height;
  int old_x, old_y;

  bool view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy);

  void focus_view(wlr_surface *surface);

  void begin_interactive(enum turbo_cursor_mode mode, uint32_t edges);

  virtual void activate() = 0;
  virtual void notify_keyboard_enter() = 0;

  virtual wlr_surface* surface_at(double sx, double sy, double *sub_x, double *sub_y) = 0;

  virtual const wlr_surface* surface() const = 0;

  virtual void set_size(int width, int height) = 0;

  virtual void geometry(wlr_box *box) const = 0;

  virtual void focus() = 0;

  virtual void toggle_maximize() = 0;

  virtual turbo_view* parent() const = 0;

  virtual bool is_child() const = 0;

  virtual void for_each_surface(wlr_surface_iterator_func_t iterator, void *data) const = 0;

  virtual float scale_output(wlr_output *output) const = 0;

  virtual void scale_coords(double inx, double iny, double *outx, double *outy) const = 0;

};

#endif
