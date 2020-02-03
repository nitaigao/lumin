#include "key_bindings/key_binding_quit.h"

#include "controller.h"

key_binding_quit::key_binding_quit(Server *server_)
  : KeyBinding()
  , server(server_)
  { }

void key_binding_quit::run() {
  server->quit();
}
