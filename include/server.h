#ifndef SERVER_H_
#define SERVER_H_

#include <xkbcommon/xkbcommon.h>
#include <wayland-server-core.h>

#include <vector>
#include <memory>
#include <string>

#include "cursor_mode.h"
#include "output.h"
#include "keyboard.h"
#include "view.h"
#include "settings.h"

struct wlr_renderer;
struct wlr_seat;
struct wlr_input_device;
struct wlr_surface;
struct wlr_backend;
struct wlr_xdg_shell;
struct wlr_cursor;
struct wlr_xcursor_manager;
struct wlr_output_layout;
struct wlr_event_pointer_button;
struct wlr_seat_pointer_request_set_cursor_event;
struct wlr_seat_request_set_primary_selection_event;
struct wlr_seat_request_set_selection_event;
struct wlr_event_pointer_axis;
struct wlr_event_pointer_motion;
struct wlr_output;

class View;
class KeyBinding;
class Output;
class Settings;

class Server {
 public:
  Server();

 public:
  void focus_top();
  void focus_view(View *view);

  void run();
  void destroy();
  void quit();

  void add_output(const std::shared_ptr<Output>& output);
  void disconnect_output(const std::string& name, bool enabled);
  void render_output(const Output *output) const;
  void remove_output(const Output *output);

  void dock_right();
  void dock_left();
  void toggle_maximize();

  void damage_outputs();

 public:
  View* view_from_surface(wlr_surface *surface);
  void maximize_view(View *view);
  void begin_interactive(View *view, CursorMode mode, unsigned int edges);
  void destroy_view(View *view);
  void position_view(View* view);

 private:
  void new_keyboard(wlr_input_device *device);
  void new_pointer(wlr_input_device *device);
  void new_switch(wlr_input_device *device);

  void process_cursor_move(uint32_t time);
  void process_cursor_resize(uint32_t time);
  void process_cursor_motion(uint32_t time);

  bool handle_key(uint32_t keycode, const xkb_keysym_t *syms, int nsyms,
    uint32_t modifiers, int state);

  View* desktop_view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy);

  void init_keybindings();

 private:
  wl_listener new_input;
  wl_listener new_output;
  wl_listener new_surface;

  wl_listener cursor_motion;
  wl_listener cursor_motion_absolute;
  wl_listener cursor_button;
  wl_listener cursor_axis;
  wl_listener cursor_frame;

  wl_listener request_cursor;
  wl_listener request_set_primary_selection;
  wl_listener request_set_selection;

  wl_listener lid;

 private:
  static void cursor_motion_absolute_notify(wl_listener *listener, void *data);
  static void cursor_motion_notify(wl_listener *listener, void *data);
  static void cursor_button_notify(wl_listener *listener, void *data);
  static void cursor_axis_notify(wl_listener *listener, void *data);
  static void cursor_frame_notify(wl_listener *listener, void *data);

  static void new_input_notify(wl_listener *listener, void *data);
  static void new_output_notify(wl_listener *listener, void *data);
  static void new_surface_notify(wl_listener *listener, void *data);

  static void keyboard_modifiers_notify(wl_listener *listener, void *data);
  static void keyboard_key_notify(wl_listener *listener, void *data);

  static void handle_request_set_primary_selection(struct wl_listener *listener, void *data);
  static void handle_request_set_selection(struct wl_listener *listener, void *data);

  static void lid_notify(wl_listener *listener, void *data);
  static void seat_request_cursor_notify(wl_listener *listener, void *data);

 private:
  std::vector<std::shared_ptr<KeyBinding>> key_bindings;

  std::vector<std::shared_ptr<Output>> outputs_;
  std::vector<std::shared_ptr<Keyboard>> keyboards_;
  std::vector<std::shared_ptr<View>> views_;

  unsigned int capabilities_;

  struct {
    View *view;
    double cursor_x, cursor_y;
    double x, y;
    double width, height;
    uint32_t resize_edges;
    enum CursorMode CursorMode;
  } grab_state_;

  struct wl_display *display_;
  wlr_xdg_shell *xdg_shell_;
  wlr_seat *seat_;
  wlr_output_layout *layout_;
  wlr_backend *backend_;
  wlr_renderer *renderer_;
  wlr_xcursor_manager *cursor_manager_;
  wlr_cursor *cursor_;

  std::unique_ptr<Settings> settings_;
};

#endif  // SERVER_H_
