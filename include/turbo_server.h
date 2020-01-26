#ifndef _TURBO_SERVER_H
#define _TURBO_SERVER_H

#include <wayland-server-core.h>

#include "turbo_cursor_mode.h"

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
struct turbo_view;

struct turbo_server {
  turbo_server();

  struct wl_display *wl_display;
  wlr_backend *backend;
  wlr_renderer *renderer;

  wlr_xdg_shell *xdg_shell;
  wlr_xwayland *xwayland;
  wl_list views;

  wlr_cursor *cursor;
  wlr_xcursor_manager *cursor_mgr;

  wl_listener new_xdg_surface;
  wl_listener new_xwayland_surface;

  wl_listener cursor_motion;
  wl_listener cursor_motion_absolute;
  wl_listener cursor_button;
  wl_listener cursor_axis;
  wl_listener cursor_frame;
  wl_listener new_input;
  wl_listener request_cursor;

  wlr_seat *seat;
  wl_list keyboards;
  enum turbo_cursor_mode cursor_mode;
  turbo_view *grabbed_view;

  double grab_x, grab_y;
  int grab_width, grab_height;
  uint32_t resize_edges;

  wlr_output_layout *output_layout;
  wl_list outputs;
  wl_listener new_output;

  turbo_view* desktop_view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy);

  void new_keyboard(wlr_input_device *device);
  void new_pointer(wlr_input_device *device);

  void process_cursor_move(uint32_t time);
  void process_cursor_resize(uint32_t time);
  void process_cursor_motion(uint32_t time);

  turbo_view* view_from_surface(wlr_surface *surface);

  void position_view(turbo_view* view);

  void pop_view(turbo_view* view);

  void run();
  void destroy();
};

#endif
