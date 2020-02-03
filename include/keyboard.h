#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <wayland-server-core.h>

struct wlr_input_device;
struct wlr_seat;
class Server;

struct Keyboard {
  Server *server;
  wlr_input_device *device;
  wlr_seat *seat;

  wl_listener modifiers;
  wl_listener key;
};

#endif  // KEYBOARD_H_
