#ifndef KEYBINDINGS_KEY_BINDING_DOCK_RIGHT_H_
#define KEYBINDINGS_KEY_BINDING_DOCK_RIGHT_H_

#include "key_binding.h"

#include <string>

struct Controller;

struct key_binding_dock_right : KeyBinding {
  explicit key_binding_dock_right(Controller *server_);

  void run();

  Controller *server;
};

#endif  // KEYBINDINGS_KEY_BINDING_DOCK_RIGHT_H_
