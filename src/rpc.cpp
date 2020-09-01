#include "rpc.h"

#include <fmt/core.h>
#include <zmqpp/zmqpp.hpp>
#include <sstream>

#include <numeric>

#include "server.h"

namespace lumin {

std::string join(const std::vector<std::string>& elems, const std::string& delim) {
  if (elems.empty()) {
    return "";
  }

  std::stringstream ss;
  auto e = elems.begin();
  ss << *e++;
  for (; e != elems.end(); ++e) {
    ss << delim << *e;
  }

  return ss.str();
}

void RPC::start(Server *server)
{
  thread_ = std::thread(&RPC::thread, server);
}

void RPC::thread(Server *server)
{
  zmqpp::context context;
  zmqpp::socket socket(context, zmqpp::socket_type::reply);
  socket.bind("tcp://*:4243");

  while (true) {
    zmqpp::message message;
    socket.receive(message);

    std::string text;
    message >> text;

    std::string reply = "unknown";

    if (text.compare("apps") == 0) {
      auto apps = server->apps();
      reply = join(apps, ",");
    }

    socket.send(reply);
  }
}

}  // namespace lumin
