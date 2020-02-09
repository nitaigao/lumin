#ifndef OUTPUT_H_
#define OUTPUT_H_

#include <wayland-server-core.h>

#include <memory>
#include <string>
#include <vector>

struct wlr_output_damage;
struct wlr_output_layout;
struct wlr_output;
struct wlr_renderer;

namespace lumin {

class Server;
class View;

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
    return wlr_output;
  }

 public:
  std::string id() const;
  int width() const;
  int height() const;

  bool is_named(const std::string& name) const;

  void init();
  void destroy();

  void set_enabled(bool enabled);
  void set_position(int x, int y);
  void set_scale(int scale);
  void set_mode();

  void add_layout(int x, int y);
  void remove_layout();

  void commit();

  void render(const std::vector<std::shared_ptr<View>>& views) const;

  void take_damage(const View *view);
  void take_whole_damage();

 private:
  static void output_destroy_notify(wl_listener *listener, void *data);
  static void output_frame_notify(wl_listener *listener, void *data);

 public:

  struct wlr_output *wlr_output;

 private:
  wlr_renderer *renderer_;
  Server *server_;
  wlr_output_damage *damage_;
  wlr_output_layout *layout_;
  bool enabled_;

 public:
  wl_listener frame_;
  wl_listener destroy_;
};

}  // namespace lumin

#endif  // OUTPUT_H_
