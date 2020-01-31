#include "keybindings/key_binding_dock_left.h"

#include "controller.h"

key_binding_dock_left::key_binding_dock_left(Controller *server_)
  : KeyBinding()
  , server(server_)
  { }

void key_binding_dock_left::run() {
  server->dock_left();
}