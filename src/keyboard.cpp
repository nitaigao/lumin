#include "keyboard.h"

#include <wlroots.h>

#include "server.h"

namespace lumin {

Keyboard::Keyboard(Server *server, wlr_input_device *device, Seat* seat)
  : server_(server)
  , device_(device)
  , seat_(seat) {
  /* Here we set up listeners for keyboard events. */
  modifiers.notify = keyboard_modifiers_notify;
  wl_signal_add(&device_->keyboard->events.modifiers, &modifiers);

  key.notify = keyboard_key_notify;
  wl_signal_add(&device_->keyboard->events.key, &key);
}

void Keyboard::setup() {
  xkb_rule_names rules;
  memset(&rules, 0, sizeof(xkb_rule_names));
  xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  xkb_keymap *keymap = xkb_map_new_from_names(context, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);

  wlr_keyboard_set_keymap(device_->keyboard, keymap);
  xkb_keymap_unref(keymap);
  xkb_context_unref(context);

  wlr_keyboard_set_repeat_info(device_->keyboard, 25, 200);
  seat_->set_keyboard(device_);
  seat_->add_capability(WL_SEAT_CAPABILITY_KEYBOARD);
}

void Keyboard::keyboard_modifiers_notify(wl_listener *listener, void *data) {
  Keyboard *keyboard = wl_container_of(listener, keyboard, modifiers);
  keyboard->seat_->set_keyboard(keyboard->device_);
  keyboard->seat_->keyboard_notify_modifiers(&keyboard->device_->keyboard->modifiers);
}

void Keyboard::keyboard_key_notify(wl_listener *listener, void *data) {
  Keyboard *keyboard = wl_container_of(listener, keyboard, key);
  auto event = static_cast<struct wlr_event_keyboard_key *>(data);

  uint32_t keycode = event->keycode + 8;
  const xkb_keysym_t *syms;
  int nsyms = xkb_state_key_get_syms(keyboard->device_->keyboard->xkb_state, keycode, &syms);
  uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->device_->keyboard);

  bool handled = keyboard->server_->handle_key(keycode, syms, nsyms, modifiers, event->state);

  if (!handled) {
    keyboard->seat_->set_keyboard(keyboard->device_);
    keyboard->seat_->keyboard_notify_key(event->time_msec, event->keycode, event->state);
  }
}

}  // namespace lumin
