#ifndef _TURBO_VIEW_XWAYLAND_H
#define _TURBO_VIEW_XWAYLAND_H

#include <wayland-server-core.h>

#include "turbo_view.h"

struct turbo_view_xwayland : public turbo_view {
  turbo_view_xwayland();

  const wlr_surface* surface() const;
  wl_listener request_configure;
  void activate();
  void notify_keyboard_enter();
  wlr_surface* surface_at(double sx, double sy, double *sub_x, double *sub_y);
  void toggle_maximize();
  void set_size(int width, int height);
  void focus();

  void for_each_surface(wlr_surface_iterator_func_t iterator, void *data) const;

  turbo_view* parent() const;
  bool is_child() const;
  void geometry(wlr_box *box) const;
  float scale_output(wlr_output *output) const;
  void scale_coords(double inx, double iny, double *outx, double *outy) const;

  wlr_xwayland_surface *xwayland_surface;
};

#endif
