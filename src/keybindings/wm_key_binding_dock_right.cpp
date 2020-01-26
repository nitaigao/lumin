#include "keybindings/wm_key_binding_dock_right.h"

#include <iostream>

#include "wm_server.h"

wm_key_binding_dock_right::wm_key_binding_dock_right(wm_server *server_)
   : wm_key_binding()
   , server(server_)
  { }

void wm_key_binding_dock_right::run() {
  server->dock_right();
}
