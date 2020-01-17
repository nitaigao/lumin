#include <output.h>

#include <iostream>
#include <algorithm>

extern "C" {
  #include <wlr/backend.h>
  #include <wlr/interfaces/wlr_output.h>
}

#include <server.h>

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

  server->destroy_output.notify = Server::destroy_output_notify;
  wl_signal_add(&wlr_output->events.destroy, &server->destroy_output);

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
  display = wl_display_create();
  event_loop = wl_event_loop_create();
  backend = wlr_backend_autocreate(display, NULL);

  new_output.notify = Server::new_output_notify;
  wl_signal_add(&backend->events.new_output, &new_output);

  if (!wlr_backend_start(backend)) {
    wl_display_destroy(display);
    throw std::runtime_error("failed to start backend");
  }
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
