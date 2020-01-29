#ifndef WM_KEYBOARD_H_
#define WM_KEYBOARD_H_

#include <wayland-server-core.h>

struct wm_server;

struct wm_keyboard {
  wl_list link;
  wm_server *server;
  wlr_input_device *device;

  wl_listener modifiers;
  wl_listener key;
};

#endif  // WM_KEYBOARD_H_
