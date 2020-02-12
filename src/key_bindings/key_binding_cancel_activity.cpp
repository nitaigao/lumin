#include "key_bindings/key_binding_cancel_activity.h"

#include "server.h"

namespace lumin {

key_binding_cancel_activity::key_binding_cancel_activity(Server *server_)
  : KeyBinding()
  , server(server_)
  { }

void key_binding_cancel_activity::run() {
  server->cancel_activity();
}

}  // namespace lumin
