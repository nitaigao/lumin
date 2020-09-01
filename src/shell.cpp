#include "shell.h"

#include <fmt/core.h>
#include <zmqpp/zmqpp.hpp>

#include "rpc.h"
#include "server.h"

namespace lumin {

zmqpp::context context;
zmqpp::socket socket(context, zmqpp::socket_type::publish);

Shell::Shell()
{
  rpc_ = std::make_shared<RPC>();
  server_ = std::make_shared<Server>();
}

void Shell::run()
{
  socket.bind("tcp://*:4242");

  server_->init();
  server_->on_key.connect_member(this, &Shell::handle_key);

  rpc_->start(server_.get());

  server_->run();
  server_->destroy();
}

void Shell::handle_key(uint keycode, uint modifiers, int state, bool *handled)
{
  zmqpp::message message;
  message << "switch";
  message << fmt::format("{} {} {}", keycode, modifiers, state);
  socket.send(message);
}

}  // namespace lumin
