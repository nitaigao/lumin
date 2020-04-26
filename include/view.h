#ifndef VIEW_H_
#define VIEW_H_

#include <wayland-server-core.h>

#include <string>

#include "cursor_mode.h"

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

enum WindowState {
  WM_WINDOW_STATE_WINDOW = 0,
  WM_WINDOW_STATE_TILED = 1,
  WM_WINDOW_STATE_MAXIMIZED = 2,
  WM_WINDOW_STATE_FULLSCREEN = 3
};

enum ViewLayer {
  VIEW_LAYER_BACKGROUND = 0,
  VIEW_LAYER_BOTTOM = 1,
  VIEW_LAYER_TOP = 2,
  VIEW_LAYER_OVERLAY = 3,
  VIEW_LAYER_MAX = 4
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
  void minimize();

  bool fullscreen() const;

  void tile_left();
  void tile_right();

  void windowize();

  void focus();
  void unfocus();

  std::string id() const;
  std::string title() const;

  void enter(const Output* output);

  uint min_width() const;
  uint min_height() const;

  bool is_child() const;
  View* parent() const;
  const View *root() const;

  bool view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy);

  bool has_surface(const wlr_surface *surface) const;
  void for_each_surface(wlr_surface_iterator_func_t iterator, void *data) const;
  wlr_surface* surface_at(double sx, double sy, double *sub_x, double *sub_y);

  bool is_launcher() const;
  bool is_menubar() const;
  bool is_shell() const;

  bool is_always_focused() const;
  bool steals_focus() const;

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
  bool minimized;
  ViewLayer layer;

 public:
  wl_listener map;
  wl_listener unmap;
  wl_listener commit;
  wl_listener destroy;
  wl_listener request_move;
  wl_listener request_resize;
  wl_listener request_maximize;
  wl_listener request_minimize;
  wl_listener request_fullscreen;
  wl_listener new_subsurface;
  wl_listener new_popup;
  wl_listener set_app_id;

 public:
  static void xdg_toplevel_request_maximize_notify(wl_listener *listener, void *data);
  static void xdg_toplevel_request_minimize_notify(wl_listener *listener, void *data);
  static void xdg_toplevel_request_fullscreen_notify(wl_listener *listener, void *data);
  static void xdg_toplevel_request_move_notify(wl_listener *listener, void *data);
  static void xdg_toplevel_request_resize_notify(wl_listener *listener, void *data);
  static void xdg_toplevel_set_app_id_notify(wl_listener *listener, void *data);

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

 private:
  Cursor *cursor_;
  wlr_output_layout *layout_;
  Seat *seat_;
  wlr_xdg_surface *xdg_surface_;
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
