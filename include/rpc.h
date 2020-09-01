#ifndef RPC_H_
#define RPC_H_

#include <thread>

namespace lumin {

class Server;

class RPC {
 public:
  void start(Server *server);

 private:
  std::thread thread_;
  static void thread(Server *server);
};

}  // namespace lumin

#endif  // RPC_H_
