#ifndef KEY_BINDINGS_KEY_BINDING_CANCEL_ACTIVITY_H_
#define KEY_BINDINGS_KEY_BINDING_CANCEL_ACTIVITY_H_

#include "key_binding.h"

#include <string>

namespace lumin {

class Server;

struct key_binding_cancel_activity : KeyBinding {
  explicit key_binding_cancel_activity(Server *server_);

  void run();

  Server *server;
};

}  // namespace lumin

#endif  // KEY_BINDINGS_KEY_BINDING_CANCEL_ACTIVITY_H_
