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
  #include <wlr/types/wlr_seat.h>
  #include <wlr/util/log.h>
}

#include <server.h>

Server::Server()
  : caps(WL_SEAT_CAPABILITY_POINTER) {

}

void Server::key_notify(struct wl_listener *listener, void *data) {
  auto event = static_cast<struct wlr_event_keyboard_key*>(data);
  uint32_t keycode = event->keycode + 8;
  std::clog << keycode << std::endl;
  if (keycode == 9) {
    exit(0);
  }
}

void Server::new_input_notify(struct wl_listener *listener, void *data) {
  std::clog << "new_input_notify" << std::endl;

  Server *server = wl_container_of(listener, server, new_input);
  auto device = static_cast<struct wlr_input_device*>(data);

  switch (device->type) {
	case WLR_INPUT_DEVICE_KEYBOARD: {
    std::clog << "WLR_INPUT_DEVICE_KEYBOARD" << std::endl;
    server->caps |= WL_SEAT_CAPABILITY_KEYBOARD;

    // struct xkb_rule_names rules;
    // struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    // struct xkb_keymap *keymap = xkb_map_new_from_names(context, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);

    // wlr_keyboard_set_keymap(device->keyboard, keymap);
    // xkb_keymap_unref(keymap);
    // xkb_context_unref(context);
    // wlr_keyboard_set_repeat_info(device->keyboard, 25, 600);

    server->key.notify = key_notify;
    wl_signal_add(&device->keyboard->events.key, &server->key);

    wlr_seat_set_keyboard(server->seat, device);

		break;
  }

	case WLR_INPUT_DEVICE_POINTER:
    std::clog << "WLR_INPUT_DEVICE_POINTER" << std::endl;
    wlr_cursor_attach_input_device(server->cursor, device);
		break;

	default:
		break;
	}

  wlr_seat_set_capabilities(server->seat, server->caps);
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
  wlr_seat_pointer_notify_frame(server->seat);
}

void Server::output_frame_notify(struct wl_listener *listener, void *data) {
  Server *server = wl_container_of(listener, server, destroy_output);

  struct wlr_output *wlr_output = static_cast<struct wlr_output*>(data);
  struct wlr_renderer *renderer = wlr_backend_get_renderer(wlr_output->backend);

  wlr_output_attach_render(wlr_output, NULL);
  wlr_renderer_begin(renderer, wlr_output->width, wlr_output->height);

  float color[4] = {1.0, 0, 0, 1.0};
  wlr_renderer_clear(renderer, color);

  wlr_output_render_software_cursors(wlr_output, NULL);

  wlr_renderer_end(renderer);
  wlr_output_commit(wlr_output);
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
  auto output = std::make_shared<Output>(wlr_output);

  if (!wl_list_empty(&wlr_output->modes)) {
		struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
		wlr_output_set_mode(wlr_output, mode);
		wlr_output_enable(wlr_output, true);
		if (!wlr_output_commit(wlr_output)) {
			return;
		}
	}

  server->destroy_output.notify = Server::destroy_output_notify;
  wl_signal_add(&wlr_output->events.destroy, &server->destroy_output);

  server->frame.notify = Server::output_frame_notify;
  wl_signal_add(&wlr_output->events.frame, &server->frame);

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
  wlr_compositor_create(display, renderer);

  // layout
  layout = wlr_output_layout_create();

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

  // seat
  seat = wlr_seat_create(display, "seat0");

  request_cursor.notify = request_cursor_notify;
	wl_signal_add(&seat->events.request_set_cursor, &request_cursor);

  // output
  new_output.notify = Server::new_output_notify;
  wl_signal_add(&backend->events.new_output, &new_output);

    // input
  new_input.notify = new_input_notify;
	wl_signal_add(&backend->events.new_input, &new_input);

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
