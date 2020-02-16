#ifndef KEY_BINDINGS_KEY_BINDING_SWITCH_APP_H_
#define KEY_BINDINGS_KEY_BINDING_SWITCH_APP_H_

#include "key_binding.h"

#include <string>

namespace lumin {

class Shell;

struct key_binding_switch_app : KeyBinding {
  explicit key_binding_switch_app(Shell *shell);

  void run();

  Shell *shell_;
};

}  // namespace lumin

#endif  // KEY_BINDINGS_KEY_BINDING_SWITCH_APP_H_
