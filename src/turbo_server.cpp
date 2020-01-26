extern "C" {
  #include <unistd.h>
  #include <wayland-server-core.h>
  #include <wlr/backend.h>
  #include <wlr/render/wlr_renderer.h>
  #include <wlr/types/wlr_cursor.h>
  #include <wlr/types/wlr_compositor.h>
  #include <wlr/types/wlr_data_device.h>
  #include <wlr/types/wlr_input_device.h>
  #include <wlr/types/wlr_keyboard.h>
  #include <wlr/types/wlr_matrix.h>
  #include <wlr/types/wlr_output.h>
  #include <wlr/types/wlr_output_layout.h>
  #include <wlr/types/wlr_pointer.h>
  #include <wlr/types/wlr_seat.h>
  #include <wlr/types/wlr_xcursor_manager.h>
  #include <wlr/types/wlr_xdg_shell.h>
  #include <wlr/util/log.h>
  #include <wlr/backend/libinput.h>
  #include <xkbcommon/xkbcommon.h>
}

#include <iostream>

#include "turbo_server.h"
#include "turbo_keyboard.h"
#include "turbo_view.h"
#include "turbo_cursor_mode.h"

turbo_server::turbo_server()
  : cursor_mode(TURBO_CURSOR_NONE) {

}

static bool handle_alt_keybinding(turbo_server *server, xkb_keysym_t sym) {
  /*
   * Here we handle compositor keybindings. This is when the compositor is
   * processing keys, rather than passing them on to the client for its own
   * processing.
   *
   * This function assumes Alt is held down.
   */
  switch (sym) {
  case XKB_KEY_Escape:
    wl_display_terminate(server->wl_display);
    break;
  default:
    return false;
  }
  return true;
}

static bool handle_ctrl_keybinding(turbo_server *server, xkb_keysym_t sym) {
  switch (sym) {

  case XKB_KEY_Return: {
    if (fork() == 0) {
      execl("/bin/sh", "/bin/sh", "-c", "tilix", (void *)NULL);
    }
    break;
  }

  default:
    return false;
  }

  return true;
}

static void keyboard_modifiers_notify(wl_listener *listener, void *data) {
  turbo_keyboard *keyboard = wl_container_of(listener, keyboard, modifiers);
  wlr_seat_set_keyboard(keyboard->server->seat, keyboard->device);
  wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,
    &keyboard->device->keyboard->modifiers);
}

static void keyboard_key_notify(wl_listener *listener, void *data) {
  /* This event is raised when a key is pressed or released. */
  turbo_keyboard *keyboard = wl_container_of(listener, keyboard, key);
  turbo_server *server = keyboard->server;
  auto event = static_cast<struct wlr_event_keyboard_key *>(data);
  wlr_seat *seat = server->seat;

  uint32_t keycode = event->keycode + 8;
  const xkb_keysym_t *syms;
  int nsyms = xkb_state_key_get_syms(keyboard->device->keyboard->xkb_state, keycode, &syms);

  bool handled = false;
  uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->device->keyboard);
  if ((modifiers & WLR_MODIFIER_ALT) && event->state == WLR_KEY_PRESSED) {
    for (int i = 0; i < nsyms; i++) {
      handled = handle_alt_keybinding(server, syms[i]);
    }
  }

  if ((modifiers & WLR_MODIFIER_CTRL) && event->state == WLR_KEY_PRESSED) {
    for (int i = 0; i < nsyms; i++) {
      handled = handle_ctrl_keybinding(server, syms[i]);
    }
  }

  for (int i = 0; i < nsyms; i++) {
    switch (syms[i]) {
    case XKB_KEY_F12:
      wl_display_terminate(server->wl_display);
      handled = true;
      break;
    }
  }

  if (!handled) {
    wlr_seat_set_keyboard(seat, keyboard->device);
    wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode, event->state);
  }
}

void turbo_server::new_keyboard(wlr_input_device *device) {
  turbo_keyboard *keyboard = new turbo_keyboard();
  keyboard->server = this;
  keyboard->device = device;

  /* We need to prepare an XKB keymap and assign it to the keyboard. This
   * assumes the defaults (e.g. layout = "us"). */
  xkb_rule_names rules;
  memset(&rules, 0, sizeof(xkb_rule_names));
  xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  xkb_keymap *keymap = xkb_map_new_from_names(context, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);

  wlr_keyboard_set_keymap(device->keyboard, keymap);
  xkb_keymap_unref(keymap);
  xkb_context_unref(context);
  wlr_keyboard_set_repeat_info(device->keyboard, 25, 600);

  /* Here we set up listeners for keyboard events. */
  keyboard->modifiers.notify = keyboard_modifiers_notify;
  wl_signal_add(&device->keyboard->events.modifiers, &keyboard->modifiers);

  keyboard->key.notify = keyboard_key_notify;
  wl_signal_add(&device->keyboard->events.key, &keyboard->key);

  wlr_seat_set_keyboard(seat, device);

  /* And add the keyboard to our list of keyboards */
  wl_list_insert(&keyboards, &keyboard->link);
}

void turbo_server::new_pointer(wlr_input_device *device) {
  bool is_libinput = wlr_input_device_is_libinput(device);
  if (is_libinput) {
    libinput_device *libinput_device = wlr_libinput_get_device_handle(device);
    libinput_device_config_tap_set_enabled(libinput_device, LIBINPUT_CONFIG_TAP_ENABLED);
    libinput_device_config_scroll_set_natural_scroll_enabled(libinput_device, 1);
    libinput_device_config_accel_set_profile(libinput_device, LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT);
  }

  wlr_cursor_attach_input_device(cursor, device);
}


turbo_view* turbo_server::desktop_view_at(double lx, double ly,
  wlr_surface **surface, double *sx, double *sy) {

  turbo_view *view;
  wl_list_for_each(view, &views, link) {

    double scaled_lx = 0;
    double scaled_ly = 0;
    view->scale_coords(lx, ly, &scaled_lx, &scaled_ly);
    if (view->view_at(scaled_lx, scaled_ly, surface, sx, sy)) {
      return view;
    }
  }
  return NULL;
}

void turbo_server::process_cursor_move(uint32_t time) {
  grabbed_view->scale_coords(cursor->x, cursor->y, &grabbed_view->x, &grabbed_view->y);
  grabbed_view->x -= grab_x;
  grabbed_view->y -= grab_y;
}

void turbo_server::process_cursor_resize(uint32_t time) {
  turbo_view *view = grabbed_view;
  double dx = 0;
  double dy = 0;
  view->scale_coords(cursor->x, cursor->y, &dx, &dy);

  dx -= grab_x;
  dy -= grab_y;

  double x = view->x;
  double y = view->y;
  int width = grab_width;
  int height = grab_height;

  if (resize_edges & WLR_EDGE_TOP) {
    y = grab_y + dy;
    height -= dy;
    if (height < 1) {
      y += height;
    }
  } else if (resize_edges & WLR_EDGE_BOTTOM) {
    height += dy;
  }

  if (resize_edges & WLR_EDGE_LEFT) {
    x = grab_x + dx;
    width -= dx;
    if (width < 1) {
      x += width;
    }
  } else if (resize_edges & WLR_EDGE_RIGHT) {
    width += dx;
  }

  view->x = x;
  view->y = y;

  view->set_size(width, height);
}

void turbo_server::process_cursor_motion(uint32_t time) {
  /* If the mode is non-passthrough, delegate to those functions. */
  if (cursor_mode == TURBO_CURSOR_MOVE) {
    process_cursor_move(time);
    return;
  } else if (cursor_mode == TURBO_CURSOR_RESIZE) {
    process_cursor_resize(time);
    return;
  }

  /* Otherwise, find the view under the pointer and send the event along. */
  double sx, sy;
  wlr_surface *surface = NULL;
  turbo_view *view = desktop_view_at(cursor->x, cursor->y, &surface, &sx, &sy);

  if (!view) {
    /* If there's no view under the cursor, set the cursor image to a
     * default. This is what makes the cursor image appear when you move it
     * around the screen, not over any views. */
    wlr_xcursor_manager_set_cursor_image(cursor_mgr, "left_ptr", cursor);
  }

  if (surface) {
    bool focus_changed = seat->pointer_state.focused_surface != surface;
    /*
     * "Enter" the surface if necessary. This lets the client know that the
     * cursor has entered one of its surfaces.
     *
     * Note that this gives the surface "pointer focus", which is distinct
     * from keyboard focus. You get pointer focus by moving the pointer over
     * a window.
     */

    wlr_seat_pointer_notify_enter(seat, surface, sx, sy);

    if (!focus_changed) {
      /* The enter event contains coordinates, so we only need to notify
       * on motion if the focus did not change. */
      wlr_seat_pointer_notify_motion(seat, time, sx, sy);
    }
  } else {
    /* Clear pointer focus so future button events and such are not sent to
     * the last client to have the cursor over it. */
    wlr_seat_pointer_clear_focus(seat);
  }
}

turbo_view* turbo_server::view_from_surface(wlr_surface *surface) {
  turbo_view *view;
  wl_list_for_each(view, &views, link) {
    if (view->surface() == surface) {
      return view;
    }
  }
  return NULL;
}

void turbo_server::position_view(turbo_view *view) {
  bool is_child = view->is_child();

  if (!is_child) {
    return;
  }

  turbo_view *parent_view = view->parent();

  wlr_box parent_geometry;
  parent_view->geometry(&parent_geometry);

  wlr_box geometry;
  view->geometry(&geometry);

  int inside_x = (parent_geometry.width - geometry.width) / 2.0;
  int inside_y = (parent_geometry.height - geometry.height) / 2.0;

  view->x = parent_view->x + inside_x;
  view->y = parent_view->y + inside_y;
}

void turbo_server::pop_view(turbo_view *old_view) {
  turbo_view *view;
  wl_list_for_each(view, &views, link) {
    if (view->mapped) {
      view->focus();
      return;
    }
  }
}
