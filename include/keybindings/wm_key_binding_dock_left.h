#ifndef _KEY_BINDING_DOCK_LEFT_H
#define _KEY_BINDING_DOCK_LEFT_H

#include "wm_key_binding.h"

#include <string>

struct wm_server;

struct wm_key_binding_dock_left : wm_key_binding {
  wm_key_binding_dock_left(wm_server *server_);

  void run();

  wm_server *server;
};

#endif
