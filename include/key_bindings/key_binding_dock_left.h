#ifndef KEY_BINDINGS_KEY_BINDING_DOCK_LEFT_H_
#define KEY_BINDINGS_KEY_BINDING_DOCK_LEFT_H_

#include "key_binding.h"

#include <string>

namespace lumin {

class Server;

struct key_binding_dock_left : KeyBinding {
  explicit key_binding_dock_left(Server *server_);

  void run();

  Server *server;
};

}  // namespace lumin

#endif  // KEY_BINDINGS_KEY_BINDING_DOCK_LEFT_H_
