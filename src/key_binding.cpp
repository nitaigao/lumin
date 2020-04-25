#include "key_binding.h"

#include <spdlog/spdlog.h>

#include <wlroots.h>

namespace lumin {

KeyBinding::KeyBinding(int key_code, int modifiers, int state)
  : key_code_(key_code)
  , modifiers_(modifiers)
  , state_(state)
{

}

bool KeyBinding::matches(int modifiers, int key_code, int state)
{
  bool match = (modifiers_ == modifiers && key_code_ == key_code && state_ == state);
  return match;
}

}  // namespace lumin
