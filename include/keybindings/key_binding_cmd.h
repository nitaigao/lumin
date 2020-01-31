#ifndef KEYBINDINGS_KEY_BINDING_CMD_H_
#define KEYBINDINGS_KEY_BINDING_CMD_H_

#include <string>

#include "key_binding.h"

struct key_binding_cmd : KeyBinding {
  std::string cmd;

  void run();
};

#endif  // KEYBINDINGS_KEY_BINDING_CMD_H_
