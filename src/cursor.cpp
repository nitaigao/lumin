#include "cursor.h"

#include <wlroots.h>

#include "seat.h"
#include "server.h"
#include "view.h"

namespace lumin {

Cursor::~Cursor()
{
  wlr_cursor_destroy(cursor_);
  wlr_xcursor_manager_destroy(cursor_manager_);
}

Cursor::Cursor(Server *server, wlr_output_layout *layout, Seat *seat)
  : server_(server)
  , layout_(layout)
  , seat_(seat)
  , grab_state_({
    .view = NULL,
    .cursor_x = 0,
    .cursor_y = 0,
    .x = 0,
    .y = 0,
    .width = 0,
    .height = 0,
    .resize_edges = 0,
    .CursorMode = WM_CURSOR_NONE
    })
{
  cursor_ = wlr_cursor_create();
  wlr_cursor_attach_output_layout(cursor_, layout_);

  cursor_manager_ = wlr_xcursor_manager_create(NULL, 24);
  wlr_xcursor_manager_load(cursor_manager_, 1);

  cursor_motion.notify = cursor_motion_notify;
  wl_signal_add(&cursor_->events.motion, &cursor_motion);

  cursor_motion_absolute.notify = cursor_motion_absolute_notify;
  wl_signal_add(&cursor_->events.motion_absolute, &cursor_motion_absolute);

  cursor_button.notify = cursor_button_notify;
  wl_signal_add(&cursor_->events.button, &cursor_button);

  cursor_axis.notify = cursor_axis_notify;
  wl_signal_add(&cursor_->events.axis, &cursor_axis);

  cursor_frame.notify = cursor_frame_notify;
  wl_signal_add(&cursor_->events.frame, &cursor_frame);
}

int Cursor::x() const
{
  return cursor_->x;
}

int Cursor::y() const
{
  return cursor_->y;
}

void Cursor::load_scale(int scale)
{
  wlr_xcursor_manager_load(cursor_manager_, scale);
}

void Cursor::warp(int x, int y)
{
  wlr_cursor_warp(cursor_, NULL, x, y);
  seat_->pointer_notify_frame();
}

void Cursor::set_surface(wlr_surface *surface, int hotspot_x, int hotspot_y)
{
  wlr_cursor_set_surface(cursor_, surface, hotspot_x, hotspot_y);
}

void Cursor::add_device(wlr_input_device* device)
{
  bool is_libinput = wlr_input_device_is_libinput(device);
  if (is_libinput) {
    libinput_device *libinput_device = wlr_libinput_get_device_handle(device);
    libinput_device_config_tap_set_enabled(libinput_device, LIBINPUT_CONFIG_TAP_ENABLED);
    libinput_device_config_scroll_set_natural_scroll_enabled(libinput_device, 1);
    libinput_device_config_accel_set_profile(libinput_device, LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT);
    libinput_device_config_accel_set_speed(libinput_device, -0.5);
  }

  wlr_cursor_attach_input_device(cursor_, device);
  seat_->add_capability(WL_SEAT_CAPABILITY_POINTER);
}

void Cursor::cursor_motion_notify(wl_listener *listener, void *data)
{
  Cursor *cursor = wl_container_of(listener, cursor, cursor_motion);
  auto event = static_cast<struct wlr_event_pointer_motion*>(data);
  wlr_cursor_move(cursor->cursor_, event->device, event->delta_x, event->delta_y);
  cursor->process_cursor_motion(event->time_msec);
}

void Cursor::cursor_motion_absolute_notify(wl_listener *listener, void *data)
{
  Cursor *cursor = wl_container_of(listener, cursor, cursor_motion_absolute);
  auto event = static_cast<struct wlr_event_pointer_motion_absolute*>(data);
  wlr_cursor_warp_absolute(cursor->cursor_, event->device, event->x, event->y);
  cursor->process_cursor_motion(event->time_msec);
}

void Cursor::process_cursor_move(uint32_t time)
{
  grab_state_.view->x = cursor_->x - grab_state_.x;
  grab_state_.view->y = cursor_->y - grab_state_.y;
}

void Cursor::process_cursor_resize(uint32_t time)
{
  View *view = grab_state_.view;

  double dx = cursor_->x - grab_state_.cursor_x;
  double dy = cursor_->y - grab_state_.cursor_y;

  double x = view->x;
  double y = view->y;

  double width = grab_state_.width;
  double height = grab_state_.height;

  if (grab_state_.resize_edges & WLR_EDGE_TOP) {
    y = grab_state_.y + dy;
    height = grab_state_.height - dy;
  } else if (grab_state_.resize_edges & WLR_EDGE_BOTTOM) {
    height += dy;
  }

  if (grab_state_.resize_edges & WLR_EDGE_LEFT) {
    x = grab_state_.x + dx;
    width -= dx;
  } else if (grab_state_.resize_edges & WLR_EDGE_RIGHT) {
    width += dx;
  }

  uint min_width = view->min_width();
  if (width > min_width) {
    view->x = x;
  }

  uint min_height = view->min_height();
  if (height > min_height) {
    view->y = y;
  }

  view->resize(width, height);
}

void Cursor::process_cursor_motion(uint32_t time)
{
  /* If the mode is non-passthrough, delegate to those functions. */
  if (grab_state_.CursorMode == WM_CURSOR_MOVE) {
    process_cursor_move(time);
    server_->damage_outputs();
    return;
  } else if (grab_state_.CursorMode == WM_CURSOR_RESIZE) {
    process_cursor_resize(time);
    return;
  }

  /* Otherwise, find the view under the pointer and send the event along. */
  double sx, sy;
  wlr_surface *surface = NULL;
  View *view = server_->desktop_view_at(cursor_->x, cursor_->y, &surface, &sx, &sy);

  if (!view) {
    /* If there's no view under the cursor, set the cursor image to a
     * default. This is what makes the cursor image appear when you move it
     * around the screen, not over any views. */
    wlr_xcursor_manager_set_cursor_image(cursor_manager_, "left_ptr", cursor_);
  }

  if (surface) {
    bool focus_changed = seat_->pointer_focused_surface() != surface;
    /*
     * "Enter" the surface if necessary. This lets the client know that the
     * cursor has entered one of its surfaces.
     *
     * Note that this gives the surface "pointer focus", which is distinct
     * from keyboard focus. You get pointer focus by moving the pointer over
     * a window.
     */
    seat_->pointer_notify_enter(surface, sx, sy);

    if (!focus_changed) {
      /* The enter event contains coordinates, so we only need to notify
       * on motion if the focus did not change. */
      seat_->pointer_motion(time, sx, sy);
    }
  } else {
    /* Clear pointer focus so future button events and such are not sent to
     * the last client to have the cursor over it. */
    seat_->pointer_clear_focus();
  }
}

void Cursor::cursor_button_notify(wl_listener *listener, void *data)
{
  Cursor *cursor = wl_container_of(listener, cursor, cursor_button);
  auto event = static_cast<struct wlr_event_pointer_button*>(data);

  cursor->seat_->pointer_notify_button(event->time_msec, event->button, event->state);

  double sx, sy;
  wlr_surface *surface;
  View *view = cursor->server_->desktop_view_at(
    cursor->cursor_->x, cursor->cursor_->y, &surface, &sx, &sy);

  if (event->state == WLR_BUTTON_RELEASED) {
    cursor->grab_state_.CursorMode = WM_CURSOR_PASSTHROUGH;
    return;
  }

  if (view != NULL) {
    cursor->server_->focus_view(view);
  }
}

void Cursor::cursor_axis_notify(wl_listener *listener, void *data)
{
  Cursor *cursor = wl_container_of(listener, cursor, cursor_axis);
  auto event = static_cast<wlr_event_pointer_axis*>(data);
  cursor->seat_->pointer_notify_axis(event->time_msec, event->orientation,
    event->delta, event->delta_discrete, event->source);
}

void Cursor::cursor_frame_notify(wl_listener *listener, void *data)
{
  Cursor *cursor = wl_container_of(listener, cursor, cursor_frame);
  cursor->seat_->pointer_notify_frame();
}

void Cursor::begin_interactive(View *view, CursorMode mode, unsigned int edges)
{
  wlr_surface *focused_surface = seat_->pointer_focused_surface();

  if (!view->has_surface(focused_surface)) {
    return;
  }

  grab_state_.x = 0;
  grab_state_.y = 0;

  grab_state_.view = view;
  grab_state_.CursorMode = mode;

  wlr_box geo_box;
  view->extents(&geo_box);

  if (mode == WM_CURSOR_MOVE) {
    grab_state_.x = cursor_->x - view->x;
    grab_state_.y = cursor_->y - view->y;
  } else {
    grab_state_.x = view->x;
    grab_state_.y = view->y;
    grab_state_.cursor_x = cursor_->x;
    grab_state_.cursor_y = cursor_->y;
  }

  grab_state_.width = geo_box.width;
  grab_state_.height = geo_box.height;

  grab_state_.resize_edges = edges;
}

}  // namespace lumin
