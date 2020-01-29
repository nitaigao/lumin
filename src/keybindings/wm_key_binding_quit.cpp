#include "keybindings/wm_key_binding_quit.h"

#include "wm_server.h"

wm_key_binding_quit::wm_key_binding_quit(wm_server *server_)
  : wm_key_binding()
  , server(server_)
  { }

void wm_key_binding_quit::run() {
  server->quit();
}
