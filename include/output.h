#ifndef OUTPUT_H_
#define OUTPUT_H_

#include <wayland-server-core.h>

#include <memory>
#include <string>
#include <vector>

#include "signal.hpp"

struct wlr_output_damage;
struct wlr_output_layout;
struct wlr_output;
struct wlr_renderer;

namespace lumin {

class View;

class Output {
 public:
  ~Output();

  explicit Output(struct wlr_output *output,
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

  bool connected() const;
  void set_connected(bool connected);

  void add_layout(int x, int y);
  void remove_layout();

  void send_enter(const std::vector<std::shared_ptr<View>>& views);

  void commit();

  void render(const std::vector<std::shared_ptr<View>>& views) const;
  void render_view(View *view) const;

  void take_damage(const View *view);
  void take_whole_damage();

  void lock_software_cursors();
  void unlock_software_cursors();

 private:
  static void output_destroy_notify(wl_listener *listener, void *data);
  static void output_frame_notify(wl_listener *listener, void *data);

 public:
  struct wlr_output *wlr_output;

 private:
  wlr_renderer *renderer_;
  wlr_output_damage *damage_;
  wlr_output_layout *layout_;
  bool enabled_;
  bool connected_;
  bool software_cursors_;
  int enter_frames_left_;

 public:
  wl_listener destroy_;
  wl_listener frame_;

 public:
  Signal<Output*> on_destroy;
  Signal<Output*> on_frame;
};

}  // namespace lumin

#endif  // OUTPUT_H_
