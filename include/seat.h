#ifndef _SEAT_H
#define _SEAT_H

#include <memory>
#include <vector>
#include <wayland-server.h>

class Keyboard;
class Mouse;

class Seat {

public:

  Seat(struct wlr_seat *seat);

public:

  void add_keyboard(const std::shared_ptr<Keyboard>& keyboard);

  void add_mouse(const std::shared_ptr<Mouse>& mouse);

  void assume_keyboard(const std::shared_ptr<Keyboard>& keyboard);

public:

  struct wlr_seat *seat;

private:

  uint32_t caps;

  std::vector<std::shared_ptr<Keyboard>> keyboards;
  std::vector<std::shared_ptr<Mouse>> mice;

};

#endif
