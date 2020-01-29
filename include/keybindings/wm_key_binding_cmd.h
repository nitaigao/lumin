#ifndef KEYBINDINGS_WM_KEY_BINDING_CMD_H_
#define KEYBINDINGS_WM_KEY_BINDING_CMD_H_

#include <string>

#include "wm_key_binding.h"

struct wm_key_binding_cmd : wm_key_binding {
  std::string cmd;

  void run();
};

#endif  // KEYBINDINGS_WM_KEY_BINDING_CMD_H_
