#ifndef WM_VIEW_H_
#define WM_VIEW_H_

#include <wayland-server-core.h>

#include "wm_cursor_mode.h"

struct wlr_xdg_surface;
struct wlr_xwayland_surface;
struct wlr_surface;
struct wlr_box;
struct wlr_output;

struct wm_server;

enum window_state {
  WM_WINDOW_STATE_WINDOW = 0,
  WM_WINDOW_STATE_TILED = 1,
  WM_WINDOW_STATE_MAXIMIZED = 2
};

typedef void (*wlr_surface_iterator_func_t)(struct wlr_surface *surface,
  int sx, int sy, void *data);

class wm_view {
 public:
  virtual ~wm_view() { }

  explicit wm_view(wm_server *server);

  wl_list link;

  wl_listener map;
  wl_listener unmap;
  wl_listener destroy;
  wl_listener request_move;
  wl_listener request_resize;
  wl_listener request_maximize;

  bool mapped;
  double x, y;
  window_state state;

  bool view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy);

  void focus_view(wlr_surface *surface);

  void begin_interactive(enum wm_cursor_mode mode, uint32_t edges);

  void map_view();

  void unmap_view();

  void tile_right();

  void tile_left();

  virtual void set_size(int width, int height) = 0;

  virtual void extends(wlr_box *box) = 0;

  virtual void activate() = 0;

  virtual void notify_keyboard_enter() = 0;

  virtual wlr_surface* surface_at(double sx, double sy, double *sub_x, double *sub_y) = 0;

  virtual const wlr_surface* surface() const = 0;

  virtual void geometry(wlr_box *box) const = 0;

  virtual void focus() = 0;

  virtual void unfocus() = 0;

  void toggle_maximized();

  virtual void maximize() = 0;

  virtual void windowify(bool restore_position) = 0;

  virtual wm_view* parent() const = 0;

  virtual bool is_child() const = 0;

  virtual void for_each_surface(wlr_surface_iterator_func_t iterator, void *data) const = 0;

  virtual float scale_output(wlr_output *output) const = 0;

  virtual void scale_coords(double inx, double iny, double *outx, double *outy) const = 0;

  virtual void tile(int edges) = 0;

  void save_geometry();

 protected:
  int old_width, old_height;
  int old_x, old_y;

  wm_server *server;
};

#endif  // WM_VIEW_H_
