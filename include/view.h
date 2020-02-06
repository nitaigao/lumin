#ifndef VIEW_H_
#define VIEW_H_

#include <wayland-server-core.h>

#include "cursor_mode.h"

struct wlr_xdg_surface;
struct wlr_xwayland_surface;
struct wlr_surface;
struct wlr_box;
struct wlr_output;
struct wlr_seat;
struct wlr_output_layout;

namespace lumin {

class Cursor;
class Seat;
class Server;

enum WindowState {
  WM_WINDOW_STATE_WINDOW = 0,
  WM_WINDOW_STATE_TILED = 1,
  WM_WINDOW_STATE_MAXIMIZED = 2
};

typedef void (*wlr_surface_iterator_func_t)(struct wlr_surface *surface,
  int sx, int sy, void *data);

class View {
 public:
  View(Server *server, wlr_xdg_surface *surface, Cursor *cursor,
     wlr_output_layout *layout, Seat *seat);

 public:
  void geometry(wlr_box *box) const;
  void extents(wlr_box *box) const;

  void resize(double width, double height);

  void toggle_maximized();
  void maximize();
  bool maximized() const;

  void tile_left();
  void tile_right();

  void window();

  void focus();
  void unfocus();

  uint min_width() const;
  uint min_height() const;

  bool is_child() const;
  View* parent() const;

  bool view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy);

  bool has_surface(const wlr_surface *surface) const;
  void for_each_surface(wlr_surface_iterator_func_t iterator, void *data) const;
  wlr_surface* surface_at(double sx, double sy, double *sub_x, double *sub_y);

 private:
  void save_geometry();
  void grab();
  void tile(int edges);
  bool tiled() const;
  bool windowed() const;
  void map_view();
  void unmap_view();
  void activate();
  void notify_keyboard_enter(wlr_seat *seat);

 public:
  bool mapped;
  double x, y;

 public:
  wl_listener map;
  wl_listener unmap;
  wl_listener commit;
  wl_listener destroy;
  wl_listener request_move;
  wl_listener request_resize;
  wl_listener request_maximize;
  wl_listener new_subsurface;
  wl_listener new_popup;

 private:
  static void xdg_toplevel_request_maximize_notify(wl_listener *listener, void *data);
  static void xdg_toplevel_request_move_notify(wl_listener *listener, void *data);
  static void xdg_toplevel_request_resize_notify(wl_listener *listener, void *data);

  static void xdg_popup_commit_notify(wl_listener *listener, void *data);
  static void xdg_popup_destroy_notify(wl_listener *listener, void *data);
  static void xdg_popup_subsurface_commit_notify(wl_listener *listener, void *data);
  static void xdg_subsurface_commit_notify(wl_listener *listener, void *data);
  static void xdg_surface_commit_notify(wl_listener *listener, void *data);
  static void xdg_surface_destroy_notify(wl_listener *listener, void *data);
  static void xdg_surface_map_notify(wl_listener *listener, void *data);
  static void xdg_surface_unmap_notify(wl_listener *listener, void *data);

  static void new_popup_notify(wl_listener *listener, void *data);
  static void new_popup_popup_notify(wl_listener *listener, void *data);
  static void new_popup_subsurface_notify(wl_listener *listener, void *data);
  static void new_subsurface_notify(wl_listener *listener, void *data);

 private:
  WindowState state;

  struct {
    int width, height;
    int x, y;
  } saved_state_;

  Server *server;
  wlr_xdg_surface *xdg_surface_;
  Cursor *cursor_;
  wlr_output_layout *layout_;
  Seat *seat_;
};

struct Subsurface {
  wl_listener commit;
  Server *server;
};

struct Popup {
  wl_listener commit;
  wl_listener destroy;
  wl_listener new_subsurface;
  wl_listener new_popup;
  Server *server;
};

}  // namespace lumin

#endif  // VIEW_H_
