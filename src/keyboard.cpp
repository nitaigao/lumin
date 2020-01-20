#include "keyboard.h"

#include <iostream>

extern "C" {
  #include <wlr/backend.h>
  #include <wlr/interfaces/wlr_output.h>
  #include <wlr/render/wlr_renderer.h>
  #include <wlr/types/wlr_compositor.h>
  #include <wlr/types/wlr_output_layout.h>
  #include <wlr/types/wlr_cursor.h>
  #include <wlr/types/wlr_xcursor_manager.h>
  #include <wlr/types/wlr_seat.h>
  #include <wlr/util/log.h>
}

#include "server.h"

Keyboard::Keyboard(struct wlr_input_device* device_, const Server* server_)
  : device(device_)
  , server(server_) {

}

void Keyboard::init() {
  std::clog << "keyboard init" << std::endl;
  struct xkb_rule_names rules;
  memset(&rules, 0, sizeof(struct xkb_rule_names));
  struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  struct xkb_keymap *keymap = xkb_map_new_from_names(context, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);

  wlr_keyboard_set_keymap(device->keyboard, keymap);
  xkb_keymap_unref(keymap);
  xkb_context_unref(context);
  wlr_keyboard_set_repeat_info(device->keyboard, 25, 600);
}
