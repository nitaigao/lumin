#ifndef KEY_BINDINGS_KEY_BINDING_CANCEL_ACTIVITY_H_
#define KEY_BINDINGS_KEY_BINDING_CANCEL_ACTIVITY_H_

#include "key_binding.h"

#include <string>

namespace lumin {

class Shell;

struct key_binding_cancel_activity : KeyBinding {
  explicit key_binding_cancel_activity(Shell *shell);

  void run();

  Shell *shell_;
};

}  // namespace lumin

#endif  // KEY_BINDINGS_KEY_BINDING_CANCEL_ACTIVITY_H_
