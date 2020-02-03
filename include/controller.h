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
struct wlr_event_pointer_button;
struct wlr_seat_pointer_request_set_cursor_event;
struct wlr_seat_request_set_primary_selection_event;
struct wlr_seat_request_set_selection_event;
struct wlr_event_pointer_axis;
class View;
class KeyBinding;
class Output;

class Server {
 public:
  Server();

  struct wl_display *display_;
  wlr_backend *backend_;
  wlr_renderer *renderer_;

  wlr_xdg_shell *xdg_shell_;

  wlr_cursor *cursor_;
  wlr_xcursor_manager *cursor_manager_;

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
  wl_listener request_set_primary_selection;
  wl_listener request_set_selection;
  wl_listener lid;
  wl_listener output_frame;

 public:
  wlr_output_layout *output_layout;
  wl_listener new_output;

  View* desktop_view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy);

  void new_keyboard(wlr_input_device *device);
  void new_pointer(wlr_input_device *device);
  void new_switch(wlr_input_device *device);

  void process_cursor_move(uint32_t time);
  void process_cursor_resize(uint32_t time);
  void process_cursor_motion(uint32_t time);

  void render_output(const Output *output) const;

  View* view_from_surface(wlr_surface *surface);

  void position_view(View* view);

  void focus_top();
  void focus_view(View *view);

  void quit();

  void run();
  void destroy();

  void destroy_view(View *view);

  void add_output(std::shared_ptr<Output>& output);
  void add_view(std::shared_ptr<View>& view);

  void disconnect_output(const std::string& name, bool enabled);
  void begin_interactive(View *view, CursorMode mode, unsigned int edges);
  void button(wlr_event_pointer_button *event);

  bool handle_key(uint32_t keycode, const xkb_keysym_t *syms, int nsyms,
    uint32_t modifiers, int state);

  void dock_right();
  void dock_left();
  void toggle_maximize();

  void remove_output(const Output *output);

  void set_cursor(wlr_seat_pointer_request_set_cursor_event *event);
  void axis(wlr_event_pointer_axis *event);
  void on_cursor_frame();
  void set_primary_selection(wlr_seat_request_set_primary_selection_event *event);
  void set_selection(wlr_seat_request_set_selection_event *event);

 public:
  void damage_outputs();

 private:
  void init_keybindings();
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

  wlr_seat *seat;
};

#endif  // SERVER_H_
