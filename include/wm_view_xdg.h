#ifndef _WM_VIEW_H
#define _WM_VIEW_H

#include "wm_view.h"

struct wm_view_xdg : public wm_view {
  wm_view_xdg();

  const wlr_surface* surface() const;
  void activate();
  void notify_keyboard_enter();
  wlr_surface* surface_at(double sx, double sy, double *sub_x, double *sub_y);
  void toggle_maximize();
  void set_size(int width, int height);
  void focus();
  void unfocus();
  void for_each_surface(wlr_surface_iterator_func_t iterator, void *data) const;
  bool is_child() const;
  wm_view* parent() const;
  void geometry(wlr_box *box) const;
  float scale_output(wlr_output *output) const;
  void scale_coords(double inx, double iny, double *outx, double *outy) const;

  wlr_xdg_surface *xdg_surface;
};

#endif
