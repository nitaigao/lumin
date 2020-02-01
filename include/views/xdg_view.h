#ifndef VIEWS_XDG_VIEW_H_
#define VIEWS_XDG_VIEW_H_

#include "view.h"

class XdgView : public View {
 public:
  XdgView(Controller* server, wlr_xdg_surface *surface);

  const wlr_surface* surface() const;

  void notify_keyboard_enter();

  void maximize();
  void tile(int edges);
  void window(bool restore_position);

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

 private:
  wlr_xdg_surface *xdg_surface;
};

#endif  // VIEWS_XDG_VIEW_H_
