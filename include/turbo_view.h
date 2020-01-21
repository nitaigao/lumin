#ifndef TURBO_VIEW_H
#define TURBO_VIEW_H

#include <wayland-server-core.h>

#include "turbo_cursor_mode.h"

struct turbo_server;
struct wlr_xdg_surface;
struct wlr_xwayland_surface;
struct wlr_surface;
struct wlr_box;

enum surface_type {
  TURBO_SURFACE_TYPE_NONE = 0,
  TURBO_XDG_SURFACE = 1,
  TURBO_XWAYLAND_SURFACE = 2
};

struct turbo_view {
  virtual ~turbo_view() {};

  wl_list link;
  turbo_server *server;
  wlr_xdg_surface *xdg_surface;
  wlr_xwayland_surface *xwayland_surface;
  enum surface_type surface_type;

  wl_listener map;
  wl_listener unmap;
  wl_listener destroy;
  wl_listener request_move;
  wl_listener request_resize;
  wl_listener request_maximize;

  virtual void activate() = 0;
  virtual void notify_keyboard_enter() = 0;
  virtual wlr_surface* surface_at(double sx, double sy, double *sub_x, double *sub_y) = 0;
  virtual void geometry(struct wlr_box *box) const = 0;

  virtual const wlr_surface* surface() const = 0;
  virtual void _maximize(int new_x, int new_y, int width, int height, bool maximized) = 0;
  virtual void set_size(int width, int heigth) = 0;

  bool mapped;
  int x, y;
  bool maximized;

  int old_width, old_height;
  int old_x, old_y;

  bool view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy);

  void focus_view(wlr_surface *surface);

  void begin_interactive(enum turbo_cursor_mode mode, uint32_t edges);

  virtual void toggle_maximize() = 0;
};

struct turbo_view_xwayland : public turbo_view {
  const wlr_surface* surface() const;
  wl_listener request_configure;
  void activate();
  void notify_keyboard_enter();
  wlr_surface* surface_at(double sx, double sy, double *sub_x, double *sub_y);
  void geometry(struct wlr_box *box) const;
  void _maximize(int new_x, int new_y, int width, int height, bool maximized);
  void toggle_maximize();
  void set_size(int width, int height);
};

struct turbo_view_xdg : public turbo_view {
  const wlr_surface* surface() const;
  void activate();
  void notify_keyboard_enter();
  wlr_surface* surface_at(double sx, double sy, double *sub_x, double *sub_y);
  void geometry(struct wlr_box *box) const;
  void _maximize(int new_x, int new_y, int width, int height, bool maximized);
  void toggle_maximize();
  void set_size(int width, int height);
};

#endif
