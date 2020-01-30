#ifndef WM_OUTPUT_H_
#define WM_OUTPUT_H_

#include <wayland-server-core.h>

struct wm_view;
struct wm_server;
struct wlr_output_damage;

struct wm_output {
  wl_list link;
  wm_server *server;
  struct wlr_output *wlr_output;
  wlr_output_damage *damage;
  wl_listener frame;
  wl_listener destroy;

  void render() const;
  void take_damage(const wm_view *view);
};

#endif  // WM_OUTPUT_H_
