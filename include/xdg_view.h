#ifndef XDG_VIEW_H
#define XDG_VIEW_H

#include <wayland-server-core.h>

#include <string>

#include "cursor_mode.h"

#include "view.h"

struct wlr_xdg_surface;
struct wlr_layer_surface_v1;
struct wlr_xwayland_surface;
struct wlr_surface;
struct wlr_box;
struct wlr_output;
struct wlr_seat;
struct wlr_output_layout;

namespace lumin {

class Cursor;
class Output;
class Seat;
class Server;

class XDGView : public View {
 public:
  XDGView(wlr_xdg_surface *surface, ICursor *cursor, wlr_output_layout *layout, Seat *seat);

 public:
  void geometry(wlr_box *box) const;
  void extents(wlr_box *box) const;

  void move(int x, int y);
  void resize(double width, double height);

  bool fullscreen() const;

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

  bool has_surface(const wlr_surface *surface) const;
  void for_each_surface(wlr_surface_iterator_func_t iterator, void *data) const;

 private:
  bool can_move() const;
  wlr_surface* surface() const;
  void activate();
  void deactivate();
  bool is_child() const;
  wlr_surface* surface_at(double sx, double sy, double *sub_x, double *sub_y);

  void notify_keyboard_enter(wlr_seat *seat);

 public:
  wl_listener commit;
  wl_listener destroy;
  wl_listener request_resize;
  wl_listener request_minimize;
  wl_listener request_fullscreen;
  wl_listener new_subsurface;
  wl_listener new_popup;

 public:
  static void xdg_toplevel_request_maximize_notify(wl_listener *listener, void *data);
  static void xdg_toplevel_request_minimize_notify(wl_listener *listener, void *data);
  static void xdg_toplevel_request_fullscreen_notify(wl_listener *listener, void *data);
  static void xdg_toplevel_request_move_notify(wl_listener *listener, void *data);
  static void xdg_toplevel_request_resize_notify(wl_listener *listener, void *data);

  static void xdg_popup_commit_notify(wl_listener *listener, void *data);
  static void xdg_popup_destroy_notify(wl_listener *listener, void *data);
  static void xdg_popup_subsurface_commit_notify(wl_listener *listener, void *data);
  static void xdg_popup_subsurface_destroy_notify(wl_listener *listener, void *data);
  static void xdg_subsurface_commit_notify(wl_listener *listener, void *data);
  static void xdg_subsurface_destroy_notify(wl_listener *listener, void *data);
  static void xdg_surface_commit_notify(wl_listener *listener, void *data);
  static void xdg_surface_destroy_notify(wl_listener *listener, void *data);
  static void xdg_surface_map_notify(wl_listener *listener, void *data);
  static void xdg_surface_unmap_notify(wl_listener *listener, void *data);

  static void new_popup_notify(wl_listener *listener, void *data);
  static void new_popup_popup_notify(wl_listener *listener, void *data);
  static void new_popup_subsurface_notify(wl_listener *listener, void *data);
  static void new_subsurface_notify(wl_listener *listener, void *data);

 private:
  wlr_xdg_surface *xdg_surface_;
};

struct Subsurface {
  wl_listener commit;
  wl_listener destroy;
  View *view;
};

struct Popup {
  wl_listener commit;
  wl_listener destroy;
  wl_listener new_subsurface;
  wl_listener new_popup;
  View *view;
};

}  // namespace lumin

#endif  // XDG_VIEW_H
