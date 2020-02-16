#include "key_bindings/key_binding_switch_app.h"

#include "shell.h"

namespace lumin {

key_binding_switch_app::key_binding_switch_app(Shell *shell)
  : KeyBinding()
  , shell_(shell)
  { }

void key_binding_switch_app::run() {
  shell_->switch_app();
}

}  // namespace lumin
