#ifndef SHELL_H_
#define SHELL_H_

#include <memory>
#include <thread>

namespace lumin {

class RPC;
class Server;

class Shell {
 public:
  Shell();

  void run();
  void handle_key(uint keycode, uint modifiers, int state, bool *handled);

 private:
  std::shared_ptr<RPC> rpc_;
  std::shared_ptr<Server> server_;
};

}  // namespace lumin

#endif  // SHELL_H_
