#ifndef WM_KEYBOARD_H_
#define WM_KEYBOARD_H_

#include <wayland-server-core.h>

struct Controller;

struct Keyboard {
  wl_list link;
  Controller *server;
  wlr_input_device *device;

  wl_listener modifiers;
  wl_listener key;
};

#endif  // WM_KEYBOARD_H_
