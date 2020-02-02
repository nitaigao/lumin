#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include <xkbcommon/xkbcommon.h>
#include <wayland-server-core.h>

#include <vector>
#include <memory>

#include "cursor_mode.h"

struct wlr_renderer;
struct wlr_seat;
struct wlr_input_device;
struct wlr_surface;
struct wlr_xwayland;
struct wlr_backend;
struct wlr_xdg_shell;
struct wlr_cursor;
struct wlr_xcursor_manager;
struct wlr_output_layout;
struct View;
struct KeyBinding;
struct Output;

class Controller {
 public:
  Controller();

  struct wl_display *wl_display;
  wlr_backend *backend;
  wlr_renderer *renderer;

  wlr_xdg_shell *xdg_shell;
  wlr_xwayland *xwayland;
  wl_list views;

  wlr_cursor *cursor;
  wlr_xcursor_manager *cursor_mgr;

 public:
  wl_listener new_input;
  wl_listener new_xdg_surface;
  wl_listener new_xwayland_surface;
  wl_listener cursor_motion;
  wl_listener cursor_motion_absolute;
  wl_listener cursor_button;
  wl_listener cursor_axis;
  wl_listener cursor_frame;
  wl_listener request_cursor;

 public:
  wlr_seat *seat;

  wl_list keyboards;
  enum CursorMode CursorMode;
  View *grabbed_view;

  double grab_cursor_x, grab_cursor_y;
  double grab_x, grab_y;
  int grab_width, grab_height;
  uint32_t resize_edges;

  wlr_output_layout *output_layout;
  wl_list outputs;
  wl_listener new_output;

  View* desktop_view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy);

  void new_keyboard(wlr_input_device *device);
  void new_pointer(wlr_input_device *device);

  void process_cursor_move(uint32_t time);
  void process_cursor_resize(uint32_t time);
  void process_cursor_motion(uint32_t time);

  View* view_from_surface(wlr_surface *surface);

  void position_view(View* view);

  void focus_top();

  void quit();

  void run();
  void destroy();

  bool handle_key(uint32_t keycode, const xkb_keysym_t *syms, int nsyms,
    uint32_t modifiers, int state);

  void dock_right();
  void dock_left();
  void toggle_maximize();


  void remove_output(const Output *output);

 public:
  void damage_outputs();

 public:

 private:
  void init_keybindings();
  std::vector<std::shared_ptr<KeyBinding>> key_bindings;
};

#endif  // CONTROLLER_H_
