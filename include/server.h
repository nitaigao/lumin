#ifndef _SERVER_H
#define _SERVER_H

#include <vector>
#include <memory>

#include <wayland-server.h>

class Output;

class Server {

public:

  void destroy();

  void start_loop();

  void run();

  void quit();

public:

  void add_output(const std::shared_ptr<Output> &output);

  void remove_output(const Output *output);

private:

  void init();

private:

  struct wl_display *display;
  struct wl_event_loop *event_loop;

  struct wlr_backend *backend;

  struct wl_listener new_output;
  struct wl_listener destroy_output;

private:

  std::vector<std::shared_ptr<Output>> outputs;

private:

  static void new_output_notify(struct wl_listener *listener, void *data);
  static void destroy_output_notify(struct wl_listener *listener, void *data);

};

#endif
