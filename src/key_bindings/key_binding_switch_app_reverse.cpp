#include "key_bindings/key_binding_switch_app_reverse.h"

#include "server.h"

namespace lumin {

key_binding_switch_app_reverse::key_binding_switch_app_reverse(Server *server_)
  : KeyBinding()
  , server(server_)
  { }

void key_binding_switch_app_reverse::run() {
  server->switch_app_reverse();
}

}  // namespace lumin
