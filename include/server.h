#ifndef SERVER_H_
#define SERVER_H_

#include <wayland-server-core.h>

#include <map>
#include <memory>
#include <string>
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

namespace lumin {

class Cursor;
class KeyBinding;
class Keyboard;
class Output;
class Seat;
class Settings;
class Shell;
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

  void focus_top();
  void focus_view(View *view);

  void add_output(const std::shared_ptr<Output>& output);
  void enable_output(const std::string& name, bool enabled);
  void render_output(Output *output) const;
  void remove_output(Output *output);
  void damage_outputs();
  void enable_builtin_screen(bool enabled);

  void dock_right();
  void dock_left();
  void toggle_maximize();

  bool handle_key(uint32_t keycode, const xkb_keysym_t *syms, int nsyms,
    uint32_t modifiers, int state);

 public:
  View* desktop_view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy);
  View* view_from_surface(wlr_surface *surface);

  void maximize_view(View *view);
  void destroy_view(View *view);
  void position_view(View* view);

  const std::vector<std::shared_ptr<View>>& views() const;

 private:
  void new_keyboard(wlr_input_device *device);
  void new_switch(wlr_input_device *device);

  void init_keybindings();
  void apply_layout();

 private:
  wl_listener new_input;
  wl_listener new_output;
  wl_listener new_surface;

  wl_listener lid;

 private:
  static void layout_changed_notify(wl_listener *listener, void *data);
  static void lid_notify(wl_listener *listener, void *data);

  static void new_input_notify(wl_listener *listener, void *data);
  static void new_output_notify(wl_listener *listener, void *data);
  static void new_surface_notify(wl_listener *listener, void *data);

 private:
  std::vector<std::shared_ptr<KeyBinding>> key_bindings;
  std::vector<std::shared_ptr<Keyboard>> keyboards_;
  std::vector<std::shared_ptr<Output>> outputs_;
  std::vector<std::shared_ptr<View>> views_;

  uint capabilities_;

  wl_display *display_;
  wlr_xdg_shell *xdg_shell_;
  wlr_output_layout *layout_;
  wlr_backend *backend_;
  wlr_renderer *renderer_;

  std::unique_ptr<Cursor> cursor_;
  std::unique_ptr<Settings> settings_;
  std::unique_ptr<Seat> seat_;
  std::unique_ptr<Shell> shell_;
};

}  // namespace lumin

#endif  // SERVER_H_
