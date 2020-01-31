#ifndef OUTPUT_H_
#define OUTPUT_H_

#include <wayland-server-core.h>

struct View;
struct Controller;
struct wlr_output_damage;

class Output {
 public:
  ~Output();

  explicit Output(Controller *server,
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
  void take_damage(const View *view);
  void take_whole_damage();

 private:
  Controller *server_;
  wlr_output_damage *damage_;
  struct wlr_output *output_;
};

#endif  // OUTPUT_H_
