#ifndef _KEY_BINDING_MAXIMIZE_H
#define _KEY_BINDING_MAXIMIZE_H

#include "wm_key_binding.h"

#include <string>

struct wm_server;

struct wm_key_binding_maximize : wm_key_binding {
  wm_key_binding_maximize(wm_server *server_);

  void run();

  wm_server *server;
};

#endif
