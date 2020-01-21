#ifndef _TURBO_SERVER_H
#define _TURBO_SERVER_H

#include <wayland-server-core.h>

#include "turbo_cursor_mode.h"

struct wlr_renderer;
struct wlr_seat;
struct wlr_input_device;
struct wlr_surface;

struct turbo_server {
  struct wl_display *wl_display;
  struct wlr_backend *backend;
  wlr_renderer *renderer;

  struct wlr_xdg_shell *xdg_shell;
  wl_listener new_xdg_surface;

  struct wlr_xwayland *xwayland;
  wl_listener new_xwayland_surface;
  struct wl_list views;

  struct wlr_cursor *cursor;
  struct wlr_xcursor_manager *cursor_mgr;
  wl_listener cursor_motion;
  wl_listener cursor_motion_absolute;
  wl_listener cursor_button;
  wl_listener cursor_axis;
  wl_listener cursor_frame;

  wlr_seat *seat;
  wl_listener new_input;
  wl_listener request_cursor;
  struct wl_list keyboards;
  enum turbo_cursor_mode cursor_mode;
  struct turbo_view *grabbed_view;

  double grab_x, grab_y;
  int grab_width, grab_height;
  uint32_t resize_edges;

  struct wlr_output_layout *output_layout;
  struct wl_list outputs;
  wl_listener new_output;

  void new_keyboard(wlr_input_device *device);
  void new_pointer(wlr_input_device *device);

  turbo_view* desktop_view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy);
  void process_cursor_move(uint32_t time);
  void process_cursor_resize(uint32_t time);
  void process_cursor_motion(uint32_t time);
};

#endif
