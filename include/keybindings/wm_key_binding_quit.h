#ifndef KEYBINDINGS_WM_KEY_BINDING_QUIT_H_
#define KEYBINDINGS_WM_KEY_BINDING_QUIT_H_

#include "wm_key_binding.h"

#include <string>

struct wm_server;

struct wm_key_binding_quit : wm_key_binding {
  explicit wm_key_binding_quit(wm_server *server_);

  void run();

  wm_server *server;
};

#endif  // KEYBINDINGS_WM_KEY_BINDING_QUIT_H_
