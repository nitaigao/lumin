#ifndef _VIEW_H
#define _VIEW_H

#include <wayland-server-core.h>

class Server;

class View {

public:

  View(const Server* server, struct wlr_xdg_surface* surface);

public:

  void map();

  bool mapped() const;

  void focus() const;

public:

  struct wl_listener map_signal;

private:

  const Server* server_;
  bool mapped_;

public:

  struct wlr_xdg_surface* xdg_surface_;

  int x;
  int y;

};

#endif
