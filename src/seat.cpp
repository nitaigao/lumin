#include "seat.h"

#include <memory>

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

#include "keyboard.h"

#include "mouse.h"

Seat::Seat(struct wlr_seat *seat_)
  : seat(seat_)
  , caps(WL_SEAT_CAPABILITY_POINTER) {

}

void Seat::assume_keyboard(const std::shared_ptr<Keyboard>& keyboard) {
  wlr_seat_set_keyboard(seat, keyboard->device);
}

void Seat::add_mouse(const std::shared_ptr<Mouse>& mouse) {
  mice.push_back(mouse);
  wlr_seat_set_capabilities(seat, caps);
}

void Seat::add_keyboard(const std::shared_ptr<Keyboard>& keyboard) {
  keyboards.push_back(keyboard);

  caps |= WL_SEAT_CAPABILITY_KEYBOARD;

  wlr_seat_set_capabilities(seat, caps);
}
