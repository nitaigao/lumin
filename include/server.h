#ifndef _SERVER_H
#define _SERVER_H

#include <vector>
#include <memory>

#include <wayland-server.h>

class Output;

class Server {

public:

  Server();

public:

  void destroy();

  void start_loop();

  void run();

  void quit();

public:

  void add_output(const std::shared_ptr<Output> &output);

  void remove_output(const Output *output);

private:

  void init();

  void move_cursor();

private:

  struct wl_display *display;
  struct wl_event_loop *event_loop;

  struct wlr_backend *backend;
  struct wlr_output_layout *layout;
  struct wlr_cursor *cursor;
  struct wlr_xcursor_manager *cursor_manager;

  struct wlr_seat *seat;

	struct wl_listener new_input;

  struct wl_listener new_output;
  struct wl_listener destroy_output;

  struct wl_listener frame;

  struct wl_listener cursor_motion;
	struct wl_listener cursor_motion_absolute;
	struct wl_listener cursor_button;
	struct wl_listener cursor_axis;
	struct wl_listener cursor_frame;

  struct wl_listener key;

	struct wl_listener request_cursor;

private:

  std::vector<std::shared_ptr<Output>> outputs;

  uint32_t caps;

private:

  static void new_input_notify(struct wl_listener *listener, void *data);

  static void new_output_notify(struct wl_listener *listener, void *data);
  static void destroy_output_notify(struct wl_listener *listener, void *data);

  static void output_frame_notify(struct wl_listener *listener, void *data);

  static void cursor_motion_notify(struct wl_listener *listener, void *data);
  static void cursor_motion_absolute_notify(struct wl_listener *listener, void *data);
  static void cursor_button_notify(struct wl_listener *listener, void *data);
  static void cursor_axis_notify(struct wl_listener *listener, void *data);
  static void cursor_frame_notify(struct wl_listener *listener, void *data);

  static void request_cursor_notify(struct wl_listener *listener, void *data);

  static void key_notify(struct wl_listener *listener, void *data);
};

#endif
