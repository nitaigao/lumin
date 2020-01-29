#ifndef WM_OUTPUT_H_
#define WM_OUTPUT_H_

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

#endif  // WM_OUTPUT_H_
