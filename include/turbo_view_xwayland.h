#ifndef _TURBO_VIEW_XWAYLAND_H
#define _TURBO_VIEW_XWAYLAND_H

#include <wayland-server-core.h>

#include "turbo_view.h"

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
  void focus();
};

#endif
