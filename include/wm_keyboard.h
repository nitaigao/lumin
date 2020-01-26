#ifndef _wm_KEYBOARD_H
#define _wm_KEYBOARD_H

#include <wayland-server-core.h>

struct wm_server;

struct wm_keyboard {
  wl_list link;
  wm_server *server;
  wlr_input_device *device;

  wl_listener modifiers;
  wl_listener key;
};

#endif
