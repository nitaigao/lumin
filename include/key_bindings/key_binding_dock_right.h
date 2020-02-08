#ifndef KEY_BINDINGS_KEY_BINDING_DOCK_RIGHT_H_
#define KEY_BINDINGS_KEY_BINDING_DOCK_RIGHT_H_

#include "key_binding.h"

#include <string>

namespace lumin {

class Server;

struct key_binding_dock_right : KeyBinding {
  explicit key_binding_dock_right(Server *server_);

  void run();

  Server *server;
};

}  // namespace lumin

#endif  // KEY_BINDINGS_KEY_BINDING_DOCK_RIGHT_H_
