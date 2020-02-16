#include "key_bindings/key_binding_cancel_activity.h"

#include "shell.h"

namespace lumin {

key_binding_cancel_activity::key_binding_cancel_activity(Shell *shell)
  : KeyBinding()
  , shell_(shell)
  { }

void key_binding_cancel_activity::run() {
  shell_->cancel_activity();
}

}  // namespace lumin
