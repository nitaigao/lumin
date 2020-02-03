#ifndef KEY_BINDINGS_KEY_BINDING_DOCK_LEFT_H_
#define KEY_BINDINGS_KEY_BINDING_DOCK_LEFT_H_

#include "key_binding.h"

#include <string>

class Controller;

struct key_binding_dock_left : KeyBinding {
  explicit key_binding_dock_left(Controller *server_);

  void run();

  Controller *server;
};

#endif  // KEY_BINDINGS_KEY_BINDING_DOCK_LEFT_H_
