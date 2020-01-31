#ifndef XWAYLAND_VIEW_H_
#define XWAYLAND_VIEW_H_

#include <wayland-server-core.h>

#include "view.h"

class XWaylandView : public View {
 public:
  XWaylandView(Controller *server, wlr_xwayland_surface *surface);

  const wlr_surface* surface() const;

  void activate();
  void notify_keyboard_enter();

  wlr_surface* surface_at(double sx, double sy, double *sub_x, double *sub_y);

  void maximize();
  void windowify(bool restore_position);

  void resize(int width, int height);

  void focus();
  void unfocus();

  void for_each_surface(wlr_surface_iterator_func_t iterator, void *data) const;

  View* parent() const;
  bool is_child() const;

  void geometry(wlr_box *box) const;

  float scale_output(wlr_output *output) const;
  void scale_coords(double inx, double iny, double *outx, double *outy) const;

  void extents(wlr_box *box);

  void tile(int edges);
  void committed();
  void save_geometry();

  wl_listener request_configure;

 private:
  wlr_xwayland_surface *xwayland_surface;
};

#endif  // XWAYLAND_VIEW_H_
