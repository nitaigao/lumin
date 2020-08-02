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
struct wlr_xcursor_manager;

namespace lumin
{
class Cursor;
class KeyBinding;
class Keyboard;
class Output;
class Seat;
class Settings;
class CompositorEndpoint;
class View;
class IPlatform;
class IOS;
class IDisplayConfig;
class IOutput;
class ICursor;

class Server
{
 public:
  ~Server();

  Server();

  Server(
    const std::shared_ptr<IPlatform> &platform,
    const std::shared_ptr<IOS> &os,
    const std::shared_ptr<IDisplayConfig> &display_config,
    const std::shared_ptr<ICursor> &cursor);

 public:
  bool init();
  void run();
  void destroy();
  void quit();

  void focus_view(View *view);
  void focus_app(const std::string &app_id);

  void enable_output(const std::string &name, bool enabled);

  void dock_right();
  void dock_left();

  void minimize_top();
  void toggle_maximize();
  void maximize_view(View *view);

  bool key(uint32_t keycode, uint32_t modifiers, int state);

  int add_keybinding(int key_code, int modifiers, int state);

 public:
  View *desktop_view_at(double lx, double ly, wlr_surface **surface, double *sx, double *sy);
  View *view_from_surface(wlr_surface *surface);

  std::vector<std::string> apps() const;

 private:
  void focus_top();

  void position_view(View *view);

  void damage_outputs();
  void damage_output(View *view);

  Output *primary_output() const;

 public:
  void view_created(const std::shared_ptr<View> &view);
  void view_mapped(View *view);
  void view_unmapped(View *view);
  void view_destroyed(View *view);

  void view_minimized(View *view);
  void view_moved(View *view);
  void view_focused(View *view);
  void view_damaged(View *view);

  void keyboard_created(const std::shared_ptr<Keyboard> &keyboard);
  void keyboard_key(uint32_t time_msec, uint32_t keycode, uint32_t modifiers, int state);

  void cursor_motion(ICursor* cursor, int x, int y, uint32_t time);
  void cursor_button(ICursor* cursor, int x, int y);

  void output_created(const std::shared_ptr<Output> &output);
  void output_destroyed(Output *output);
  void output_frame(Output *output);
  void output_mode(Output *output);
  void outputs_changed(IOutput *output);

  void lid_switch(bool enabled);

 private:
  static void dbus_thread(Server *server);

 private:
  static void purge_deleted_views(void *data);
  static void purge_deleted_outputs(void *data);

 public:
  std::map<uint, KeyBinding> key_bindings;

  std::vector<std::shared_ptr<Keyboard>> keyboards_;
  std::vector<std::shared_ptr<IOutput>> outputs_;
  std::vector<std::shared_ptr<View>> views_;

  std::shared_ptr<Seat> seat_;
  std::shared_ptr<IPlatform> platform_;
  std::shared_ptr<IOS> os_;
  std::shared_ptr<IDisplayConfig> display_config_;
  std::shared_ptr<ICursor> cursor_;

  std::shared_ptr<Settings> settings_;
  std::unique_ptr<CompositorEndpoint> endpoint_;

  std::thread dbus_;
};

}  // namespace lumin

#endif  // SERVER_H_
