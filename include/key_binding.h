#ifndef KEY_BINDING_H_
#define KEY_BINDING_H_

#include <wlr/types/wlr_keyboard.h>

class KeyBinding {
 public:
  virtual ~KeyBinding() { }

  KeyBinding();

  bool ctrl;
  bool alt;
  bool super;
  unsigned int key;
  wlr_key_state state;

  int mods() const;

  bool matches(int modifiers, unsigned int sym, wlr_key_state Key_state);

  virtual void run() = 0;
};


#endif  // KEY_BINDING_H_
