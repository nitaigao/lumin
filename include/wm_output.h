#ifndef WM_OUTPUT_H_
#define WM_OUTPUT_H_

#include <wayland-server-core.h>

struct wm_view;
struct wm_server;
struct wlr_output_damage;

class wm_output {
 public:
  ~wm_output();

  explicit wm_output(wm_server *server,
                     struct wlr_output *output,
                     wlr_output_damage *damage);

 public:
  const wlr_output_damage* damage() const {
    return damage_;
  }

  struct wlr_output* output() const {
    return output_;
  }

 public:
  wl_listener frame_;
  wl_listener destroy_;

  wl_list link;

 public:
  void destroy();
  void render() const;
  void take_damage(const wm_view *view);
  void take_whole_damage();

 private:
  wm_server *server_;
  wlr_output_damage *damage_;
  struct wlr_output *output_;
};

#endif  // WM_OUTPUT_H_
