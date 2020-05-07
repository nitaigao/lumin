#ifndef SERVER_H_
#define SERVER_H_

#include <wayland-server-core.h>

#include <map>
#include <memory>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "cursor_mode.h"

typedef uint32_t xkb_keysym_t;

struct wlr_backend;
struct wlr_display;
struct wlr_output_layout;
struct wlr_renderer;
struct wlr_xdg_shell;
struct wlr_surface;
struct wlr_input_device;
struct wlr_output_manager_v1;
struct wlr_data_control_manager_v1;
struct wlr_xwayland;
struct wlr_xcursor_manager;

namespace lumin {

class Cursor;
class KeyBinding;
class Keyboard;
class Output;
class Seat;
class Settings;
class CompositorEndpoint;
class View;

class Server {
 public:
  ~Server();
  Server();

 public:
  void init();
  void run();
  void destroy();
  void quit();

  void focus_view(View *view);
  void focus_app(const std::string& app_id);

  void add_output(const std::shared_ptr<Output>& output);
  void enable_output(const std::string& name, bool enabled);
  void enable_builtin_screen(bool enabled);

  void dock_right();
  void dock_left();

  void minimize_top();
  void toggle_maximize();
  void maximize_view(View *view);
  void minimize_view(View *view);

  bool handle_key(uint32_t keycode, uint32_t modifiers, int state);

  int add_keybinding(int key_code, int modifiers, int state);

  std::vector<std::shared_ptr<View>> mapped_views() const;

 public:
  View* desktop_view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy);
  View* view_from_surface(wlr_surface *surface);

  std::vector<std::string> apps() const;

 private:
  void new_keyboard(wlr_input_device *device);
  void new_switch(wlr_input_device *device);

  void apply_layout();
  void focus_top();

  void position_view(View* view);
  void destroy_view(View *view);

  void damage_outputs();
  void damage_output(View *view);

  void render_output(Output *output) const;
  void remove_output(Output *output);
  Output* primary_output() const;
  Output* output_at(int x, int y) const;

 private:
  wl_listener new_input;
  wl_listener new_output;
  wl_listener new_xdg_surface;
  wl_listener new_xwayland_surface;

  wl_listener lid_toggle;

 private:
  static void layout_changed_notify(wl_listener *listener, void *data);
  static void lid_toggle_notify(wl_listener *listener, void *data);

  static void new_input_notify(wl_listener *listener, void *data);
  static void new_output_notify(wl_listener *listener, void *data);
  static void new_xdg_surface_notify(wl_listener *listener, void *data);
  static void new_xwayland_surface_notify(wl_listener *listener, void *data);

 private:
  void view_mapped(View *view);
  void view_unmapped(View *view);
  void view_minimized(View *view);
  void view_damaged(View *view);
  void view_destroyed(View *view);
  void view_moved(View *view);

  void output_destroyed(Output *output);
  void output_frame(Output *output);
  void output_mode(Output *output);

  void cursor_moved(Cursor *cursor, int x, int y, uint32_t time);
  void cursor_button(Cursor *cursor, int x, int y);

  void keyboard_key(uint32_t time_msec, uint32_t keycode, uint32_t modifiers, int state);

 private:

  static void dbus_thread(Server *server);

 private:

  static void purge_deleted_views(void *data);

 private:
  std::map<uint, KeyBinding> key_bindings;
  std::vector<std::shared_ptr<Keyboard>> keyboards_;
  std::vector<std::shared_ptr<Output>> outputs_;
  std::vector<std::shared_ptr<View>> views_;

  uint capabilities_;

  wl_display *display_;
  wlr_xdg_shell *xdg_shell_;
  wlr_output_layout *layout_;
  wlr_backend *backend_;
  wlr_renderer *renderer_;
  wlr_output_manager_v1 *output_manager_;
  wlr_xwayland *xwayland_;
  wlr_xcursor_manager *xcursor_manager_;

  std::unique_ptr<Cursor> cursor_;
  std::unique_ptr<Settings> settings_;
  std::unique_ptr<Seat> seat_;

  std::unique_ptr<CompositorEndpoint> endpoint_;

  std::thread dbus_;
};

}  // namespace lumin

#endif  // SERVER_H_
