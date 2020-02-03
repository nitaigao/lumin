#include "key_bindings/key_binding_maximize.h"

#include "server.h"

key_binding_maximize::key_binding_maximize(Server *server_)
  : KeyBinding()
  , server(server_)
  { }

void key_binding_maximize::run() {
  server->toggle_maximize();
}
