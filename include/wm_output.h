#ifndef wm_OUTPUT_H
#define wm_OUTPUT_H

#include <wayland-server-core.h>

struct wm_server;

struct wm_output {
  wl_list link;
  wm_server *server;
  struct wlr_output *wlr_output;
  wl_listener frame;
  wl_listener destroy;

  void render() const;
};

#endif
