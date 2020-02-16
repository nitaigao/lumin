#ifndef SHELL_H_
#define SHELL_H_

#include <memory>
#include <thread>

namespace lumin {

class Server;
class Switcher;

class Shell {
 public:
  ~Shell();
  Shell(Server *server);

 public:
  void run();
  void switch_app();
  void switch_app_reverse();
  void cancel_activity();

 private:
  void init();
  static void thread(void *data);

  std::thread ui_thread_;
  std::unique_ptr<Switcher> switcher_;
};

}  // namespace lumin

#endif  // SHELL_H_
