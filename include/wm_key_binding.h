#ifndef _KEY_BINDING_H
#define _KEY_BINDING_H

#include <wlr/types/wlr_keyboard.h>

struct wm_key_binding {
  virtual ~wm_key_binding() { }

  wm_key_binding();

  bool ctrl;
  bool alt;
  bool super;
  unsigned int key;
  wlr_key_state state;

  int mods() const;

  bool matches(int modifiers, unsigned int sym, wlr_key_state Key_state);

  virtual void run() = 0;
};


#endif
