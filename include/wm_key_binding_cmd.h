#ifndef _KEY_BINDING_CMD_H
#define _KEY_BINDING_CMD_H

#include <string>

#include "wm_key_binding.h"

struct wm_key_binding_cmd : wm_key_binding {
  std::string cmd;

  void run();
};

#endif
