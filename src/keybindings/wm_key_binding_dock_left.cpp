#include "keybindings/wm_key_binding_dock_left.h"

#include "wm_server.h"

wm_key_binding_dock_left::wm_key_binding_dock_left(wm_server *server_)
  : wm_key_binding()
  , server(server_)
  { }

void wm_key_binding_dock_left::run() {
  server->dock_left();
}
