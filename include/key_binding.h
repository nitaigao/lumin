#ifndef KEY_BINDING_H_
#define KEY_BINDING_H_

#include <wlr/types/wlr_keyboard.h>

namespace lumin {

class KeyBinding {
 public:
  virtual ~KeyBinding() { }

  KeyBinding();

  bool ctrl;
  bool shift;
  bool alt;
  bool super;
  unsigned int key;
  wlr_key_state state;

  int mods() const;

  bool matches(int modifiers, unsigned int sym, wlr_key_state Key_state);

  virtual void run() = 0;
};

}  // namespace lumin


#endif  // KEY_BINDING_H_
