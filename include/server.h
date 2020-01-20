#ifndef _SERVER_H
#define _SERVER_H

#include <vector>
#include <memory>

#include <wayland-server.h>

class Output;
class Keyboard;
class Seat;
class View;
class IHandler;

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

  void add_input_device(struct wlr_input_device *device);

  void add_view(const std::shared_ptr<View>& view);

  void render_output(const Output* output) const;

  void focus_view(const View* view) const;

private:

  void init();

  void move_cursor();

  void attach_keyboard(const std::shared_ptr<Keyboard>& keyboard);

public:

  struct wlr_output_layout *layout;

private:

  struct wl_display *display;
  struct wl_event_loop *event_loop;

  struct wlr_backend *backend;
  struct wlr_cursor *cursor;
  struct wlr_xcursor_manager *cursor_manager;

  struct wlr_xdg_shell *xdg_shell;

  std::shared_ptr<Seat> seat;

  std::vector<std::shared_ptr<IHandler>> handlers_;

  // struct wlr_seat *seat;

	struct wl_listener new_input;

  struct wl_listener new_output;
  struct wl_listener destroy_output;

  struct wl_listener cursor_motion;
	struct wl_listener cursor_motion_absolute;
	struct wl_listener cursor_button;
	struct wl_listener cursor_axis;
	struct wl_listener cursor_frame;

	struct wl_listener request_cursor;
  struct wl_listener new_xdg_surface;

private:

  std::vector<std::shared_ptr<Output>> outputs;
  std::vector<std::shared_ptr<Keyboard>> keyboards;
  std::vector<std::shared_ptr<View>> views;

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

  static void new_xdg_surface_notify(struct wl_listener *listener, void *data);
  static void xdg_surface_map_notify(struct wl_listener *listener, void* data);

  static void request_cursor_notify(struct wl_listener *listener, void *data);

  static void key_notify(struct wl_listener *listener, void *data);
  static void modifiers_notify(struct wl_listener *listener, void *data);

};

#endif
