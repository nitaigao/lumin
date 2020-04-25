#ifndef KEY_BINDING_H_
#define KEY_BINDING_H_

#include <wlr/types/wlr_keyboard.h>

namespace lumin {

class KeyBinding {
 public:
  virtual ~KeyBinding() { }

  KeyBinding(int key_code, int modifiers, int state);

  int key_code_;
  int modifiers_;
  int state_;

  bool matches(int modifiers, int key_code, int state);
};

}  // namespace lumin


#endif  // KEY_BINDING_H_
