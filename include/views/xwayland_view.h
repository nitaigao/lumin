#ifndef XWAYLAND_VIEW_H_
#define XWAYLAND_VIEW_H_

#include "view.h"

class XWaylandView : public View {
 public:
  XWaylandView(Controller *server, wlr_xwayland_surface *surface);

  const wlr_surface* surface() const;

  void notify_keyboard_enter();

  void maximize();
  void tile(int edges);
  void windowify(bool restore_position);

  void resize(int width, int height);

  void focus();
  void unfocus();

  void for_each_surface(wlr_surface_iterator_func_t iterator, void *data) const;
  wlr_surface* surface_at(double sx, double sy, double *sub_x, double *sub_y);

  bool is_child() const;
  View* parent() const;

  void geometry(wlr_box *box) const;
  void extents(wlr_box *box);

  float scale_output(wlr_output *output) const;
  void scale_coords(double inx, double iny, double *outx, double *outy) const;

  void committed();

 private:
  void save_geometry();
  void activate();

 public:
  wl_listener request_configure;

 private:
  wlr_xwayland_surface *xwayland_surface;
};

#endif  // XWAYLAND_VIEW_H_
