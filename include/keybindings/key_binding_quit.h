#ifndef KEYBINDINGS_KEY_BINDING_QUIT_H_
#define KEYBINDINGS_KEY_BINDING_QUIT_H_

#include "key_binding.h"

#include <string>

struct Controller;

struct key_binding_quit : KeyBinding {
  explicit key_binding_quit(Controller *server_);

  void run();

  Controller *server;
};

#endif  // KEYBINDINGS_KEY_BINDING_QUIT_H_
