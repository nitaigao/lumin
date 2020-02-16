#ifndef SWITCHER_H_
#define SWITCHER_H_

#include <memory>

namespace lumin {

class Server;
class SwitcherView;

class Switcher {
 public:
  Switcher(Server *server);
 public:
  void init();
  void next();
  void previous();
  void hide();
 private:
  std::unique_ptr<SwitcherView> view_;
  Server *server_;
  int selected_index_;
};

}  // namespace lumin

#endif  // SWITCHER_H_
