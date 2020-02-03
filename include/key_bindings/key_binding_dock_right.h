#ifndef KEY_BINDINGS_KEY_BINDING_DOCK_RIGHT_H_
#define KEY_BINDINGS_KEY_BINDING_DOCK_RIGHT_H_

#include "key_binding.h"

#include <string>

class Controller;

struct key_binding_dock_right : KeyBinding {
  explicit key_binding_dock_right(Controller *server_);

  void run();

  Controller *server;
};

#endif  // KEY_BINDINGS_KEY_BINDING_DOCK_RIGHT_H_
