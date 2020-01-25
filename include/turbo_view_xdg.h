#ifndef _TURBO_VIEW_H
#define _TURBO_VIEW_H

#include "turbo_view.h"

struct turbo_view_xdg : public turbo_view {
  const wlr_surface* surface() const;
  void activate();
  void notify_keyboard_enter();
  wlr_surface* surface_at(double sx, double sy, double *sub_x, double *sub_y);
  void geometry(struct wlr_box *box) const;
  void toggle_maximize();
  void set_size(int width, int height);
  void focus();
};

#endif
