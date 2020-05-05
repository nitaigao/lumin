#ifndef XWAYLAND_VIEW_H_
#define XWAYLAND_VIEW_H_

#include <wayland-server-core.h>

#include <string>

#include "cursor_mode.h"
#include "signal.hpp"

#include "view.h"

namespace lumin {

class XWaylandView : public View {
 public:
  ~XWaylandView() {}

  XWaylandView(wlr_xwayland_surface *surface, Cursor *cursor, wlr_output_layout *layout, Seat *seat);

 public:
  void geometry(wlr_box *box) const;
  void extents(wlr_box *box) const;

  void move(int x, int y);
  void resize(double width, double height);

  void focus();
  void unfocus();

  std::string id() const;
  std::string title() const;

  void enter(const Output* output);

  uint min_width() const;
  uint min_height() const;

  void set_tiled(int edges);
  void set_maximized(bool maximized);
  void set_size(int width, int height);

  bool is_root() const;
  View* parent() const;
  const View *root() const;

  void tile(int edges);

  bool has_surface(const wlr_surface *surface) const;
  void for_each_surface(wlr_surface_iterator_func_t iterator, void *data) const;

  wlr_surface* surface_at(double sx, double sy, double *sub_x, double *sub_y);

  bool steals_focus() const;

 private:
  void activate();
  void deactivate();
  wlr_surface* surface() const;

 private:
  wlr_xwayland_surface *xwayland_surface_;

  wl_listener request_resize;

 private:
  static void xwayland_surface_commit_notify(wl_listener *listener, void *data);
  static void xwayland_surface_map_notify(wl_listener *listener, void *data);
  static void xwayland_surface_unmap_notify(wl_listener *listener, void *data);
  static void xwayland_request_move_notify(wl_listener *listener, void *data);
  static void xwayland_request_resize_notify(wl_listener *listener, void *data);
  static void xwayland_request_configure_notify(wl_listener *listener, void *data);
  static void xwayland_request_maximize_notify(wl_listener *listener, void *data);
};

}  // namespace lumin

#endif  // XWAYLAND_VIEW_H_
