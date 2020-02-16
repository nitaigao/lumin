#include "key_bindings/key_binding_switch_app_reverse.h"

#include "shell.h"

namespace lumin {

key_binding_switch_app_reverse::key_binding_switch_app_reverse(Shell *shell)
  : KeyBinding()
  , shell_(shell)
  { }

void key_binding_switch_app_reverse::run() {
  shell_->switch_app_reverse();
}

}  // namespace lumin
