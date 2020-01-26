#ifndef _KEY_BINDING_QUIT_H
#define _KEY_BINDING_QUIT_H

#include "wm_key_binding.h"

#include <string>

struct wm_server;

struct wm_key_binding_quit : wm_key_binding {
  wm_key_binding_quit(wm_server *server_);

  void run();

  wm_server *server;
};

#endif
