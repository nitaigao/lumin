#ifndef WLROOTS_PLATFORM_H
#define WLROOTS_PLATFORM_H

#include "iplatform.h"

#include <wayland-server-core.h>
#include <memory>

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

namespace lumin {

class Cursor;
class Seat;

class WlRootsPlatform : public IPlatform {

 public:
  bool init();
  bool start();
  void run();
  void destroy();
  void terminate();

  std::shared_ptr<Seat> seat() const;
  std::shared_ptr<Cursor> cursor() const;

  void add_idle(idle_func func, void *data);

  Output* output_at(int x, int y) const;

 private:
  wl_display *display_;
  wlr_backend *backend_;
  wlr_renderer *renderer_;
  wlr_xdg_shell *xdg_shell_;
  wlr_output_layout *layout_;
  wlr_xcursor_manager *xcursor_manager_;
  wlr_output_manager_v1 *output_manager_;

  std::shared_ptr<Seat> seat_;
  std::shared_ptr<Cursor> cursor_;

 private:
  static void new_input_notify(wl_listener *listener, void *data);
  static void new_output_notify(wl_listener *listener, void *data);
  static void new_surface_notify(wl_listener *listener, void *data);
  static void toggle_lid_notify(wl_listener *listener, void *data);

 private:
  void handle_cursor_button(Cursor *cursor, int x, int y);
  void handle_cursor_moved(Cursor *cursor, int x, int y, uint32_t time);

 private:
  void new_keyboard(wlr_input_device *device);
  void new_switch(wlr_input_device *device);

 private:
  wl_listener new_input;
  wl_listener new_output;
  wl_listener new_surface;
  wl_listener lid_toggle;
};

}  // namespace lumin

#endif
