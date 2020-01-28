#ifndef KEYBINDINGS_WM_KEY_BINDING_DOCK_RIGHT_H_
#define KEYBINDINGS_WM_KEY_BINDING_DOCK_RIGHT_H_

#include "wm_key_binding.h"

#include <string>

struct wm_server;

struct wm_key_binding_dock_right : wm_key_binding {
  explicit wm_key_binding_dock_right(wm_server *server_);

  void run();

  wm_server *server;
};

#endif  // KEYBINDINGS_WM_KEY_BINDING_DOCK_RIGHT_H_
