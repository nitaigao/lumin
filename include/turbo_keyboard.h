#ifndef _TURBO_KEYBOARD_H
#define _TURBO_KEYBOARD_H

#include <wayland-server-core.h>

struct turbo_server;

struct turbo_keyboard {
  wl_list link;
  turbo_server *server;
  wlr_input_device *device;

  wl_listener modifiers;
  wl_listener key;
};

#endif
