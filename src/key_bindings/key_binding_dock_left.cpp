#include "key_bindings/key_binding_dock_left.h"

#include "server.h"

namespace lumin {

key_binding_dock_left::key_binding_dock_left(Server *server_)
  : KeyBinding()
  , server(server_)
  { }

void key_binding_dock_left::run() {
  server->dock_left();
}

}  // namespace lumin
