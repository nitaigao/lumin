#include "keybindings/wm_key_binding_maximize.h"

#include "wm_server.h"

wm_key_binding_maximize::wm_key_binding_maximize(wm_server *server_)
  : wm_key_binding()
  , server(server_)
  { }

void wm_key_binding_maximize::run() {
  server->maximize();
}
