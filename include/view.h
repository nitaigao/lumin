#ifndef VIEW_H_
#define VIEW_H_

#include <wayland-server-core.h>

#include <string>

#include "cursor_mode.h"
#include "signal.hpp"

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
  virtual ~View() { }
  View(Cursor *cursor, wlr_output_layout *layout, Seat *seat);

 public:
  virtual void geometry(wlr_box *box) const = 0;
  virtual void extents(wlr_box *box) const = 0;

  virtual void move(int x, int y) = 0;
  virtual void resize(double width, double height) = 0;

  void toggle_maximized();
  void maximize();
  bool maximized() const;
  void minimize();
  bool fullscreen() const;

  bool tiled() const;
  void tile_left();
  void tile_right();

  void focus();
  void unfocus();

  ViewLayer layer() const;

  virtual std::string id() const = 0;
  virtual std::string title() const = 0;

  virtual void enter(const Output* output) = 0;

  virtual uint min_width() const = 0;
  virtual uint min_height() const = 0;

  virtual bool is_root() const = 0;
  virtual View* parent() const = 0;
  virtual const View *root() const = 0;

  virtual void set_tiled(int edges) = 0;
  virtual void set_maximized(bool maximized) = 0;
  virtual void set_size(int width, int height) = 0;

  bool view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy);

  virtual bool has_surface(const wlr_surface *surface) const = 0;
  virtual void for_each_surface(wlr_surface_iterator_func_t iterator, void *data) const = 0;
  virtual wlr_surface* surface_at(double sx, double sy, double *sub_x, double *sub_y) = 0;

  bool is_launcher() const;
  bool is_menubar() const;
  bool is_shell() const;

  void grab();

  void windowize();
  bool windowed() const;

  void tile(int edges);
  void save_geometry();

  virtual void activate() = 0;
  virtual void deactivate() = 0;

  virtual wlr_surface* surface() const = 0;

  bool steals_focus() const;
  bool is_always_focused() const;

 public:
  Signal<View*> on_map;
  Signal<View*> on_unmap;
  Signal<View*> on_minimize;
  Signal<View*> on_damage;
  Signal<View*> on_destroy;
  Signal<View*> on_move;
  Signal<View*> on_commit;

 public:
  bool mapped;
  double x, y;
  bool minimized;
  bool deleted;

 protected:
  WindowState state;

  struct {
    int width, height;
    int x, y;
  } saved_state_;

 protected:
  Cursor *cursor_;
  wlr_output_layout *layout_;
  Seat *seat_;

 public:
  static const int MENU_HEIGHT = 27;

 protected:
  wl_listener map;
  wl_listener unmap;
  wl_listener commit;
  wl_listener request_move;
  wl_listener request_configure;
  wl_listener request_maximize;
};

}  // namespace lumin

#endif  // VIEW_H_
