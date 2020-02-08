#ifndef KEY_BINDINGS_KEY_BINDING_MAXIMIZE_H_
#define KEY_BINDINGS_KEY_BINDING_MAXIMIZE_H_

#include "key_binding.h"

#include <string>

namespace lumin {

class Server;

struct key_binding_maximize : KeyBinding {
  explicit key_binding_maximize(Server *server_);

  void run();

  Server *server;
};

}  // namespace lumin

#endif  // KEY_BINDINGS_KEY_BINDING_MAXIMIZE_H_
