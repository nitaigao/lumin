#ifndef OUTPUT_H_
#define OUTPUT_H_

#include <wayland-server-core.h>

#include <memory>
#include <vector>

class View;
class Server;
struct wlr_output_damage;
struct wlr_output_layout;
struct wlr_renderer;

class Output {
 public:
  ~Output();

  explicit Output(Server *server,
                  struct wlr_output *output,
                  wlr_renderer *renderer,
                  wlr_output_damage *damage,
                  wlr_output_layout *layout);

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
  void render(const std::vector<std::shared_ptr<View>>& views) const;
  void take_damage(const View *view);
  void take_whole_damage();

  void frame();

 public:
  struct wlr_output *output_;

 private:
  wlr_renderer *renderer_;
  Server *server_;
  wlr_output_damage *damage_;
  wlr_output_layout *layout_;
};

#endif  // OUTPUT_H_
