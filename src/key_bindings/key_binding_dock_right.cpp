#include "key_bindings/key_binding_dock_right.h"

#include "server.h"

namespace lumin {

key_binding_dock_right::key_binding_dock_right(Server *server_)
  : KeyBinding()
  , server(server_)
  { }

void key_binding_dock_right::run() {
  server->dock_right();
}

}  // namespace lumin
