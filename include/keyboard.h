#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <wayland-server-core.h>

struct wlr_input_device;

namespace lumin {

class Seat;
class Server;

class Keyboard {
 public:
  Keyboard(Server *server, wlr_input_device *device, Seat* seat);

 public:
  void setup();

 private:
  static void keyboard_key_notify(wl_listener *listener, void *data);
  static void keyboard_modifiers_notify(wl_listener *listener, void *data);

 public:
  wl_listener modifiers;
  wl_listener key;

 private:
  Server *server_;
  wlr_input_device *device_;
  Seat *seat_;
};

}

#endif  // KEYBOARD_H_
