#ifndef KEY_BINDINGS_KEY_BINDING_CMD_H_
#define KEY_BINDINGS_KEY_BINDING_CMD_H_

#include <string>

#include "key_binding.h"

namespace lumin {

struct key_binding_cmd : KeyBinding {
  std::string cmd;

  void run();
};

}  // namespace lumin

#endif  // KEY_BINDINGS_KEY_BINDING_CMD_H_
