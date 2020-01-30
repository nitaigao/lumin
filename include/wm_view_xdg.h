#ifndef WM_VIEW_XDG_H_
#define WM_VIEW_XDG_H_

#include "wm_view.h"

class wm_view_xdg : public wm_view {
 public:
  wm_view_xdg(wm_server* server, wlr_xdg_surface *surface);

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
  bool is_child() const;
  wm_view* parent() const;
  void geometry(wlr_box *box) const;
  float scale_output(wlr_output *output) const;
  void scale_coords(double inx, double iny, double *outx, double *outy) const;
  void extends(wlr_box *box);
  void tile(int edges);
  void committed();
  void save_geometry();

 private:
  wlr_xdg_surface *xdg_surface;
};

#endif  // WM_VIEW_XDG_H_
