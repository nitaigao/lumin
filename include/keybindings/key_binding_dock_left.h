#ifndef WM_KEYBINDINGS_KEY_BINDING_DOCK_LEFT_H_
#define WM_KEYBINDINGS_KEY_BINDING_DOCK_LEFT_H_

#include "key_binding.h"

#include <string>

struct Controller;

struct key_binding_dock_left : KeyBinding {
  explicit key_binding_dock_left(Controller *server_);

  void run();

  Controller *server;
};

#endif  // WM_KEYBINDINGS_KEY_BINDING_DOCK_LEFT_H_
