#include "seat.h"

#include <wlroots.h>

#include "cursor.h"
#include "server.h"

namespace lumin {

Seat::Seat(Server *server, wlr_seat *seat)
  : server_(server)
  , seat_(seat)
  , cursor_(NULL)
  , capabilities_(0)
{
  request_cursor.notify = seat_request_cursor_notify;
  wl_signal_add(&seat_->events.request_set_cursor, &request_cursor);

  request_set_selection.notify = handle_request_set_selection;
  wl_signal_add(&seat_->events.request_set_selection, &request_set_selection);

  request_set_primary_selection.notify = handle_request_set_primary_selection;
  wl_signal_add(&seat_->events.request_set_primary_selection, &request_set_primary_selection);
}

void Seat::add_capability(unsigned int capability) {
  capabilities_ |= capability;
  wlr_seat_set_capabilities(seat_, capabilities_);
}

void Seat::pointer_notify_frame() {
  wlr_seat_pointer_notify_frame(seat_);
}

void Seat::seat_request_cursor_notify(wl_listener *listener, void *data) {
  Seat *seat = wl_container_of(listener, seat, request_cursor);
  auto event = static_cast<wlr_seat_pointer_request_set_cursor_event*>(data);
  wlr_seat_client *focused_client = seat->seat_->pointer_state.focused_client;

  if (focused_client == event->seat_client) {
    seat->cursor_->set_surface(event->surface, event->hotspot_x, event->hotspot_y);
  }
}

void Seat::handle_request_set_selection(struct wl_listener *listener, void *data) {
  Seat *seat = wl_container_of(listener, seat, request_set_selection);
  auto event = static_cast<wlr_seat_request_set_selection_event*>(data);
  wlr_seat_set_selection(seat->seat_, event->source, event->serial);
}

void Seat::handle_request_set_primary_selection(struct wl_listener *listener, void *data) {
  Seat *seat = wl_container_of(listener, seat, request_set_primary_selection);
  auto event = static_cast<wlr_seat_request_set_primary_selection_event*>(data);
  wlr_seat_set_primary_selection(seat->seat_, event->source, event->serial);
}

void Seat::set_keyboard(wlr_input_device *device) {
  wlr_seat_set_keyboard(seat_, device);
}

void Seat::set_pointer(Cursor *cursor) {
  cursor_ = cursor;
}

void Seat::pointer_motion(uint32_t time_msec, double sx, double sy) {
  wlr_seat_pointer_notify_motion(seat_, time_msec, sx, sy);
}

void Seat::pointer_clear_focus() {
  wlr_seat_pointer_clear_focus(seat_);
}

void Seat::pointer_notify_enter(wlr_surface *surface, double sx, double sy) {
  wlr_seat_pointer_notify_enter(seat_, surface, sx, sy);
}

void Seat::keyboard_notify_modifiers(wlr_keyboard_modifiers *modifiers) {
  wlr_seat_keyboard_notify_modifiers(seat_, modifiers);
}

wlr_surface* Seat::pointer_focused_surface() const {
  return seat_->pointer_state.focused_surface;
}

wlr_surface* Seat::keyboard_focused_surface() const {
  return seat_->keyboard_state.focused_surface;
}

void Seat::pointer_notify_button(uint32_t time_msec, uint32_t button, unsigned int state) {
  wlr_seat_pointer_notify_button(seat_, time_msec, button, (wlr_button_state)state);
}

void Seat::keyboard_notify_key(uint32_t time_msec, uint32_t button, unsigned int state) {
  wlr_seat_keyboard_notify_key(seat_, time_msec, button, state);
}

void Seat::pointer_notify_axis(uint32_t time_msec, unsigned int orientation, double delta,
    int value_discrete, unsigned int source) {
    wlr_seat_pointer_notify_axis(seat_, time_msec,
      (wlr_axis_orientation)orientation, delta, value_discrete,
      (wlr_axis_source)source);
}

void Seat::keyboard_notify_enter(wlr_surface *surface) {
  wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat_);
  wlr_seat_keyboard_notify_enter(seat_, surface,
    keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
}

}  // namespace lumin
