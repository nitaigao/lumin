#ifndef KEY_BINDINGS_KEY_BINDING_SWITCH_APP_REVERSE_H_
#define KEY_BINDINGS_KEY_BINDING_SWITCH_APP_REVERSE_H_

#include "key_binding.h"

#include <string>

namespace lumin {

class Server;

struct key_binding_switch_app_reverse : KeyBinding {
  explicit key_binding_switch_app_reverse(Server *server_);

  void run();

  Server *server;
};

}  // namespace lumin

#endif  // KEY_BINDINGS_KEY_BINDING_SWITCH_APP_REVERSE_H_
