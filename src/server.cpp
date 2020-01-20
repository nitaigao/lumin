#include <output.h>

#include <iostream>
#include <algorithm>

extern "C" {
  #include <wlr/backend.h>
  #include <wlr/interfaces/wlr_output.h>
  #include <wlr/render/wlr_renderer.h>
  #include <wlr/types/wlr_compositor.h>
  #include <wlr/types/wlr_output_layout.h>
  #include <wlr/types/wlr_cursor.h>
  #include <wlr/types/wlr_xcursor_manager.h>
  #include <wlr/types/wlr_data_device.h>
  #include <wlr/types/wlr_seat.h>
  #include <wlr/types/wlr_xdg_shell.h>
  #include <wlr/xwayland.h>
  #include <wlr/util/log.h>
}

#include <unistd.h>

#include "seat.h"
#include "server.h"
#include "keyboard.h"
#include "mouse.h"
#include "view.h"

Server::Server() {

}

void Server::modifiers_notify(struct wl_listener *listener, void *data) {
  Keyboard *keyboard = wl_container_of(listener, keyboard, modifiers);

  wlr_seat_set_keyboard(keyboard->server->seat->seat, keyboard->device);
  wlr_seat_keyboard_notify_modifiers(keyboard->server->seat->seat, &keyboard->device->keyboard->modifiers);
}

void Server::key_notify(struct wl_listener *listener, void *data) {
  auto event = static_cast<struct wlr_event_keyboard_key*>(data);

  Keyboard *keyboard = wl_container_of(listener, keyboard, key);
  uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->device->keyboard);

  uint32_t keycode = event->keycode + 8;
  const xkb_keysym_t *syms;
	int nsyms = xkb_state_key_get_syms(keyboard->device->keyboard->xkb_state, keycode, &syms);

  for (int i = 0; i < nsyms; i++) {

    if ((modifiers & WLR_MODIFIER_CTRL) && event->state == WLR_KEY_PRESSED) {
      if (syms[i] == XKB_KEY_Escape) {
          exit(0);
      }
      if (syms[i] == XKB_KEY_Return) {
        std::clog << "return" << std::endl;
        if (fork() == 0) {
          execl("/bin/sh", "/bin/sh", "-c", "gnome-terminal", (void *)NULL);
        }
      }
    }
  }

  wlr_seat_set_keyboard(keyboard->server->seat->seat, keyboard->device);
	wlr_seat_keyboard_notify_key(keyboard->server->seat->seat, event->time_msec, event->keycode, event->state);
}

class IHandler { };

class Handler : public IHandler {

protected:

  Handler(const Server *server);

public:

  static void handle(struct wl_listener *listener, void *data);

private:

  virtual void _handle(void *data) = 0;

  struct wl_listener listener;
  struct wl_signal signal;

protected:

  const Server* server_;

};

Handler::Handler(const Server *server)
  : server_(server) {

}

void Handler::handle(struct wl_listener *listener, void *data) {
  Handler *handler = wl_container_of(listener, handler, signal);
  handler->_handle(data);
}

class KeyboardModifiersHandler : public Handler {

public:

  KeyboardModifiersHandler(
    const Server *server,
    Keyboard *keyboard,
    const Seat *seat,
    struct wl_signal *signal
  )
    : Handler(server)
    , keyboard_(keyboard)
    , seat_(seat)
  {
    keyboard->key.notify = handle;
    wl_signal_add(signal, &keyboard->key);
  }

private:

  void _handle(void *data);

private:

  const Keyboard *keyboard_;
  const Seat *seat_;

};

void KeyboardModifiersHandler::_handle(void *data) {
  seat->keyboard_notify_modifiers(keyboard_->modifiers());
  wlr_seat_keyboard_notify_modifiers(seat->seat, &keyboard_->device->keyboard->modifiers);
}

void Server::add_input_device(struct wlr_input_device* device) {
  switch (device->type) {

	case WLR_INPUT_DEVICE_KEYBOARD: {
    auto keyboard = std::make_shared<Keyboard>(device, this);
    keyboard->init();

    seat->add_keyboard(keyboard);
    seat->assume_keyboard(keyboard);

    auto handler = std::make_shared<KeyboardModifiersHandler>(this, keyboard.get(), seat.get(), device->keyboard);
    handlers_.push_back(handler);

    // keyboard->modifiers.notify = modifiers_notify;
    // wl_signal_add(&device->keyboard->events.modifiers, &keyboard->modifiers);

		break;
  }

	case WLR_INPUT_DEVICE_POINTER: {
    std::clog << "WLR_INPUT_DEVICE_POINTER" << std::endl;
    auto mouse = std::make_shared<Mouse>(device);
    seat->add_mouse(mouse);
    wlr_cursor_attach_input_device(cursor, device);
		break;
  }

	default:
		break;
	}
}

void Server::new_input_notify(struct wl_listener *listener, void *data) {
  std::clog << "new_input_notify" << std::endl;

  Server *server = wl_container_of(listener, server, new_input);
  auto device = static_cast<struct wlr_input_device*>(data);

  server->add_input_device(device);
}

void Server::request_cursor_notify(struct wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, request_cursor);
  auto event = static_cast<struct wlr_seat_pointer_request_set_cursor_event*>(data);

  wlr_cursor_set_surface(server->cursor, event->surface, event->hotspot_x, event->hotspot_y);
}

void Server::move_cursor() {
  wlr_xcursor_manager_set_cursor_image(cursor_manager, "left_ptr", cursor);
}

void Server::cursor_motion_notify(struct wl_listener *listener, void *data) {
  std::clog << "cursor_motion_notify" << std::endl;
  Server *server = wl_container_of(listener, server, cursor_motion);
  auto event = static_cast<struct wlr_event_pointer_motion*>(data);

  wlr_cursor_move(server->cursor, event->device, event->delta_x, event->delta_y);
  server->move_cursor();
}

void Server::cursor_motion_absolute_notify(struct wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, cursor_motion_absolute);
  auto event = static_cast<struct wlr_event_pointer_motion_absolute*>(data);
	wlr_cursor_warp_absolute(server->cursor, event->device, event->x, event->y);
  server->move_cursor();
}

void Server::cursor_button_notify(struct wl_listener *listener, void *data) {
  std::clog << "cursor_button_notify" << std::endl;
}

void Server::cursor_axis_notify(struct wl_listener *listener, void *data) {
  std::clog << "cursor_axis_notify" << std::endl;
}

void Server::cursor_frame_notify(struct wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, cursor_frame);

  wlr_seat_pointer_notify_frame(server->seat->seat);
}

void Server::render_output(const Output* output) const {
  output->render(views);
}

void Server::output_frame_notify(struct wl_listener *listener, void *data) {
  Output *output = wl_container_of(listener, output, frame);
  output->server->render_output(output);
}

void Server::add_view(const std::shared_ptr<View>& view) {
  views.push_back(view);
}

void Server::focus_view(const View* view) const {
  std::clog << "focus_view" << std::endl;
  wlr_xdg_toplevel_set_activated(view->xdg_surface_, true);
  struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat->seat);
  wlr_seat_keyboard_notify_enter(seat->seat, view->xdg_surface_->surface, keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
}

void Server::xdg_surface_map_notify(struct wl_listener *listener, void* data) {
  std::clog << "xdg_surface_map_notify" << std::endl;

  View *view = wl_container_of(listener, view, map_signal);
  view->map();
  view->focus();
}

void Server::new_xdg_surface_notify(struct wl_listener *listener, void* data) {
  auto xdg_surface = static_cast<struct wlr_xdg_surface*>(data);

  if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
		return;
	}

  Server *server = wl_container_of(listener, server, new_xdg_surface);

  auto view = std::make_shared<View>(server, xdg_surface);
  view->map_signal.notify = Server::xdg_surface_map_notify;
  wl_signal_add(&xdg_surface->events.map, &view->map_signal);

  server->add_view(view);
}

void Server::destroy_output_notify(struct wl_listener *listener, void* data) {
  Server *server = wl_container_of(listener, server, destroy_output);

  struct wlr_output *wlr_output = static_cast<struct wlr_output*>(data);
  Output* output = static_cast<Output*>(wlr_output->data);
  server->remove_output(output);
}

void Server::new_output_notify(struct wl_listener *listener, void* data) {
  Server *server = wl_container_of(listener, server, new_output);

  struct wlr_output *wlr_output = static_cast<struct wlr_output*>(data);
  auto output = std::make_shared<Output>(wlr_output, server);

  if (!wl_list_empty(&wlr_output->modes)) {
		struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
		wlr_output_set_mode(wlr_output, mode);
		wlr_output_enable(wlr_output, true);
		if (!wlr_output_commit(wlr_output)) {
			return;
		}
	}

  if (wlr_output->width >= 3000) {
    wlr_output_set_scale(wlr_output, 2);
  }

  server->destroy_output.notify = Server::destroy_output_notify;
  wl_signal_add(&wlr_output->events.destroy, &server->destroy_output);

  output->frame.notify = Server::output_frame_notify;
  wl_signal_add(&wlr_output->events.frame, &output->frame);

  wlr_output_layout_add_auto(server->layout, wlr_output);

  wlr_output_create_global(wlr_output);

  server->add_output(output);
}

void Server::add_output(const std::shared_ptr<Output> &output) {
  outputs.push_back(output);
}

void Server::remove_output(const Output* output) {
  auto remove = std::remove_if(outputs.begin(), outputs.end(), [output](auto const& element){ return element.get() == output; });
  outputs.erase(remove);

  if (outputs.empty()) {
    quit();
  }
}

void Server::quit() {
  wl_display_terminate(display);
}

void Server::init() {
  wlr_log_init(WLR_DEBUG, NULL);

  // primitives
  display = wl_display_create();
  event_loop = wl_event_loop_create();
  backend = wlr_backend_autocreate(display, NULL);

  // renderer
  auto renderer = wlr_backend_get_renderer(backend);
  wlr_renderer_init_wl_display(renderer, display);

  // compositor
  auto compositor = wlr_compositor_create(display, renderer);

  xdg_shell = wlr_xdg_shell_create(display);
  new_xdg_surface.notify = new_xdg_surface_notify;
	wl_signal_add(&xdg_shell->events.new_surface, &new_xdg_surface);

  // layout
  layout = wlr_output_layout_create();

  // output
  new_output.notify = Server::new_output_notify;
  wl_signal_add(&backend->events.new_output, &new_output);

  // cursor
  cursor = wlr_cursor_create();
  wlr_cursor_attach_output_layout(cursor, layout);

  cursor_manager = wlr_xcursor_manager_create(NULL, 48);
  wlr_xcursor_manager_load(cursor_manager, 1);

  cursor_motion.notify = cursor_motion_notify;
	wl_signal_add(&cursor->events.motion, &cursor_motion);

	cursor_motion_absolute.notify = cursor_motion_absolute_notify;
	wl_signal_add(&cursor->events.motion_absolute, &cursor_motion_absolute);

	cursor_button.notify = cursor_button_notify;
	wl_signal_add(&cursor->events.button, &cursor_button);

	cursor_axis.notify = cursor_axis_notify;
	wl_signal_add(&cursor->events.axis, &cursor_axis);

	cursor_frame.notify = cursor_frame_notify;
	wl_signal_add(&cursor->events.frame, &cursor_frame);

  // input
  new_input.notify = new_input_notify;
	wl_signal_add(&backend->events.new_input, &new_input);

  // seat
  struct wlr_seat *wlr_seat = wlr_seat_create(display, "seat0");
  seat = std::make_shared<Seat>(wlr_seat);

  request_cursor.notify = request_cursor_notify;
	wl_signal_add(&seat->seat->events.request_set_cursor, &request_cursor);

  // backend
  const char *socket = wl_display_add_socket_auto(display);

  if (!wlr_backend_start(backend)) {
    wl_display_destroy(display);
    throw std::runtime_error("failed to start backend");
  }

  printf("Running compositor on wayland display '%s'\n", socket);
  setenv("WAYLAND_DISPLAY", socket, true);

  // managers
  wl_display_init_shm(display);
  wlr_data_device_manager_create(display);

  wlr_xwayland_create(display, compositor, true);
  // wlr_gamma_control_manager_create(display);
  // wlr_screenshooter_create(display);
  // wlr_primary_selection_device_manager_create(display);
  // wlr_idle_create(display);
}

void Server::destroy() {
  wlr_backend_destroy(backend);
  wl_event_loop_destroy(event_loop);
  wl_display_destroy(display);
}

void Server::start_loop() {
  wl_display_run(display);
}

void Server::run() {
  init();
  start_loop();
  destroy();
}
