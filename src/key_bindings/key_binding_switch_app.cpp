#include "key_bindings/key_binding_switch_app.h"

#include "server.h"

namespace lumin {

key_binding_switch_app::key_binding_switch_app(Server *server_)
  : KeyBinding()
  , server(server_)
  { }

void key_binding_switch_app::run() {
  server->switch_app();
}

}  // namespace lumin
