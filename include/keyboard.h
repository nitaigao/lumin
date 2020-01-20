#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include <wayland-server.h>

class Server;

class Keyboard {

  public:

  Keyboard(struct wlr_input_device* device, const Server* server);

  public:

  void init();

  private:

  struct wlr_input_device *device;

  public:

  struct wl_listener key;
  struct wl_listener modifiers;

  const Server* server;

  friend class Server;
  friend class Seat;

};

#endif
