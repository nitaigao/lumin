#ifndef IPLATFORM_H_
#define IPLATFORM_H_

#include <memory>

#include "signal.hpp"

namespace lumin {

class ICursor;
class Output;
class Keyboard;
class Seat;
class View;

typedef void (*idle_func)(void* data);

class IPlatform {
 public:
  virtual ~IPlatform() { }

 public:
  virtual bool init() = 0;
  virtual bool start() = 0;
  virtual void run() = 0;
  virtual void destroy() = 0;
  virtual void terminate() = 0;

  virtual std::shared_ptr<Seat> seat() const = 0;
  virtual std::shared_ptr<ICursor> cursor() const = 0;

  virtual void add_idle(idle_func function, void *data) = 0;
  virtual Output* output_at(int x, int y) const = 0;

 public:
  Signal<const std::shared_ptr<Keyboard>&> on_new_keyboard;
  Signal<const std::shared_ptr<Output>&> on_new_output;
  Signal<const std::shared_ptr<View>&> on_new_view;

  Signal<bool> on_lid_switch;
};

}  // namespace lumin

#endif  // IPLATFORM_H_
