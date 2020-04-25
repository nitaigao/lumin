#ifndef SEAT_H_
#define SEAT_H_

#include <wayland-server-core.h>

struct wlr_input_device;
struct wlr_keyboard_modifiers;
struct wlr_seat;
struct wlr_surface;

namespace lumin {

class Cursor;

class Seat {
 public:
  Seat(wlr_seat *seat);

 public:
  void add_capability(unsigned int capability);

  void set_keyboard(wlr_input_device *device);
  wlr_surface* keyboard_focused_surface() const;

  void keyboard_notify_enter(wlr_surface *surface);
  void keyboard_notify_key(uint32_t time_msec, uint32_t button, unsigned int state);
  void keyboard_notify_modifiers(wlr_keyboard_modifiers *modifiers);

  void set_pointer(Cursor *cursor);
  wlr_surface* pointer_focused_surface() const;

  void pointer_clear_focus();
  void pointer_motion(uint32_t time_msec, double sx, double sy);
  void pointer_notify_axis(uint32_t time_msec, unsigned int orientation,
    double delta, int value_discrete, unsigned int source);
  void pointer_notify_button(uint32_t time_msec, uint32_t button, unsigned int state);
  void pointer_notify_enter(struct wlr_surface *surface, double sx, double sy);
  void pointer_notify_frame();

 private:
  static void handle_request_set_primary_selection(struct wl_listener *listener, void *data);
  static void handle_request_set_selection(struct wl_listener *listener, void *data);
  static void seat_request_cursor_notify(wl_listener *listener, void *data);

 public:
  wl_listener request_cursor;
  wl_listener request_set_primary_selection;
  wl_listener request_set_selection;

 private:
  wlr_seat *seat_;
  Cursor *cursor_;
  unsigned int capabilities_;
};

}

#endif  // SEAT_H_
