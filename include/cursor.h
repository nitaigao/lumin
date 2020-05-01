#ifndef CURSOR_H_
#define CURSOR_H_

#include <wayland-server-core.h>

#include "signal.hpp"
#include "cursor_mode.h"

struct wlr_cursor;
struct wlr_input_device;
struct wlr_output_layout;
struct wlr_seat;
struct wlr_surface;
struct wlr_xcursor_manager;

namespace lumin {

class Seat;
class Server;
class View;

class Cursor {
 public:
  ~Cursor();
  Cursor(wlr_output_layout *layout, Seat *seat);

 public:
  void load_scale(int scale);
  void warp(int x, int y);
  void set_image(const std::string& name);
  void set_surface(wlr_surface *surface, int hotspot_x, int hotspot_y);
  void begin_interactive(View *view, CursorMode mode, unsigned int edges);
  void add_device(wlr_input_device* device);

  int x() const;
  int y() const;

 public:
  wl_listener cursor_axis;
  wl_listener cursor_button;
  wl_listener cursor_frame;
  wl_listener cursor_motion_absolute;
  wl_listener cursor_motion;

 private:
  void process_cursor_motion(uint32_t time);
  void process_cursor_move(uint32_t time);
  void process_cursor_resize(uint32_t time);

 private:
  wlr_xcursor_manager *cursor_manager_;
  wlr_cursor *cursor_;

  wlr_output_layout *layout_;
  Seat *seat_;

  struct {
    View *view;
    double cursor_x, cursor_y;
    double x, y;
    double width, height;
    uint32_t resize_edges;
    enum CursorMode CursorMode;
  } grab_state_;

 private:
  static void cursor_motion_absolute_notify(wl_listener *listener, void *data);
  static void cursor_motion_notify(wl_listener *listener, void *data);
  static void cursor_button_notify(wl_listener *listener, void *data);
  static void cursor_axis_notify(wl_listener *listener, void *data);
  static void cursor_frame_notify(wl_listener *listener, void *data);

 public:
  Signal<Cursor*, int, int, uint32_t> on_move;
  Signal<Cursor*, int, int> on_button;
};

}

#endif  // CURSOR_H_
