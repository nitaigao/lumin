#ifndef WM_VIEW_XWAYLAND_H_
#define WM_VIEW_XWAYLAND_H_

#include <wayland-server-core.h>

#include "wm_view.h"

class wm_view_xwayland : public wm_view {
 public:
  wm_view_xwayland(wm_server *server, wlr_xwayland_surface *surface);

  const wlr_surface* surface() const;

  void activate();
  void notify_keyboard_enter();

  wlr_surface* surface_at(double sx, double sy, double *sub_x, double *sub_y);

  void maximize();
  void windowify(bool restore_position);

  void set_size(int width, int height);

  void focus();
  void unfocus();

  void for_each_surface(wlr_surface_iterator_func_t iterator, void *data) const;

  wm_view* parent() const;
  bool is_child() const;

  void geometry(wlr_box *box) const;

  float scale_output(wlr_output *output) const;
  void scale_coords(double inx, double iny, double *outx, double *outy) const;

  void extends(wlr_box *box);

  void tile(int edges);
  void committed();
  void save_geometry();

  wl_listener request_configure;

 private:
  wlr_xwayland_surface *xwayland_surface;
};

#endif  // WM_VIEW_XWAYLAND_H_
