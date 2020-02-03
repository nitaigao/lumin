#ifndef KEY_BINDINGS_KEY_BINDING_MAXIMIZE_H_
#define KEY_BINDINGS_KEY_BINDING_MAXIMIZE_H_

#include "key_binding.h"

#include <string>

class Controller;

struct key_binding_maximize : KeyBinding {
  explicit key_binding_maximize(Controller *server_);

  void run();

  Controller *server;
};

#endif  // KEY_BINDINGS_KEY_BINDING_MAXIMIZE_H_
