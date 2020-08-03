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
struct wlr_box;

namespace lumin {

class Cursor;
class View;

class IOutput {
 public:
  virtual ~IOutput() { }

 public:
  virtual std::string id() const = 0;
  virtual void configure(int scale, bool primary, bool enabled, int x, int y) = 0;
  virtual void take_damage(const View *view) = 0;
  virtual void take_whole_damage() = 0;
  virtual bool is_named(const std::string& name) const = 0;
  virtual bool connected() const = 0;
  virtual void set_connected(bool connected) = 0;
  virtual bool deleted() const = 0;
  virtual void mark_deleted() = 0;
  virtual bool primary() const = 0;
  virtual int width() const = 0;
};

class Output : public IOutput {
 public:
  ~Output();

  Output();

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

  int x() const;
  int y() const;

  void set_menubar(View *view);
  void add_view(View *view);
  void move_view(View *view, double x, double y);
  void maximize_view(View *view);
  void place_cursor(Cursor *cursor);

  bool is_named(const std::string& name) const;

  void init();

  void set_enabled(bool enabled);
  void set_position(int x, int y);
  void set_scale(int scale);
  void set_mode();

  wlr_box* box() const;

  void configure(int scale, bool primary, bool enabled, int x, int y);

  bool connected() const;
  void set_connected(bool connected);

  bool primary() const;
  void set_primary(bool primary);

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
  int top_margin() const;
  bool deleted() const;
  void mark_deleted();

 private:
  static void output_destroy_notify(wl_listener *listener, void *data);
  static void output_frame_notify(wl_listener *listener, void *data);
  static void output_mode_notify(wl_listener *listener, void *data);

 public:
  struct wlr_output *wlr_output;
  bool deleted_;

 private:
  wlr_renderer *renderer_;
  wlr_output_damage *damage_;
  wlr_output_layout *layout_;
  bool enabled_;
  bool connected_;
  bool primary_;
  bool software_cursors_;
  int enter_frames_left_;

 public:
  wl_listener destroy_;
  wl_listener frame_;
  wl_listener mode_;

 public:
  Signal<Output*> on_destroy;
  Signal<Output*> on_frame;
  Signal<Output*> on_mode;
  Signal<IOutput*> on_connect;
  Signal<IOutput*> on_disconnect;
};

}  // namespace lumin

#endif  // OUTPUT_H_
