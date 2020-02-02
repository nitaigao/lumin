#ifndef VIEW_H_
#define VIEW_H_

#include <wayland-server-core.h>

#include "cursor_mode.h"

struct wlr_xdg_surface;
struct wlr_xwayland_surface;
struct wlr_surface;
struct wlr_box;
struct wlr_output;

struct Controller;

enum WindowState {
  WM_WINDOW_STATE_WINDOW = 0,
  WM_WINDOW_STATE_TILED = 1,
  WM_WINDOW_STATE_MAXIMIZED = 2
};

typedef void (*wlr_surface_iterator_func_t)(struct wlr_surface *surface,
  int sx, int sy, void *data);

class View {
 public:
  View(Controller *server, wlr_xdg_surface *surface);

 public:
  void map_view();
  void unmap_view();

  void resize(double width, double height);

  void toggle_maximized();
  void maximize();
  bool maximized() const;

  void tile(int edges);
  bool tiled() const;

  void window(bool restore_position);
  bool windowed() const;

  void tile_right();
  void tile_left();

  void focus();
  void unfocus();

  uint min_width() const;
  uint min_height() const;

  bool is_child() const;
  View* parent() const;

  void geometry(wlr_box *box) const;
  void extents(wlr_box *box) const;

  const wlr_surface* surface() const;

  void focus_view(wlr_surface *surface);
  bool view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy);

  void committed();
  void begin_interactive(enum CursorMode mode, uint32_t edges);
  void notify_keyboard_enter();

  void for_each_surface(wlr_surface_iterator_func_t iterator, void *data) const;
  wlr_surface* surface_at(double sx, double sy, double *sub_x, double *sub_y);

 private:
  void activate();
  void save_geometry();

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

 public:
  wl_list link;
  bool mapped;
  double x, y;

 protected:
  WindowState state;

  int old_width, old_height;
  int old_x, old_y;

 public:
  Controller *server;

//  private:
  wlr_xdg_surface *xdg_surface;
};

struct Subsurface {
  wl_listener commit;
  Controller *server;
};

struct Popup {
  wl_listener commit;
  wl_listener destroy;
  wl_listener new_subsurface;
  wl_listener new_popup;
  Controller *server;
};

#endif  // VIEW_H_
