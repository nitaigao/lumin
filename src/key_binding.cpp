#include "key_binding.h"

#include <wlr/types/wlr_keyboard.h>

KeyBinding::KeyBinding()
    : ctrl(false)
    , alt(false)
    , super(false)
    , key(0)
    , state(WLR_KEY_PRESSED)
  { }

int KeyBinding::mods() const {
  int modifiers = 0;
  if (ctrl) {
    modifiers |= WLR_MODIFIER_CTRL;
  }
  if (alt) {
    modifiers |= WLR_MODIFIER_ALT;
  }
  if (super) {
    modifiers |= WLR_MODIFIER_LOGO;
  }
  return modifiers;
}

bool KeyBinding::matches(int modifiers, unsigned int sym, wlr_key_state key_state) {
  if (mods() & modifiers && sym == key && state == key_state) {
    return true;
  }
  return false;
}
