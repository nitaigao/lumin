#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <wayland-server-core.h>

struct wlr_input_device;
class Server;

struct Keyboard {
  wl_list link;
  Server *server;
  wlr_input_device *device;

  wl_listener modifiers;
  wl_listener key;
};

#endif  // KEYBOARD_H_
