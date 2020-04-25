#ifndef WINDOW_H_
#define WINDOW_H_

#include "window_adapter.h"
#include "server.h"

namespace lumin {

class WindowEndpoint : public org::os::Compositor::Window_adaptor,
  public DBus::IntrospectableAdaptor,
  public DBus::ObjectAdaptor
{
 public:
  WindowEndpoint(DBus::Connection &connection, Server *server)
    : DBus::ObjectAdaptor(connection, "/org/os/Compositor/Window")
    , server_(server) { }

  std::vector<std::string> Apps() {
    auto apps = server_->apps();
    auto results = std::vector<std::string>(apps.begin(), apps.end());
    return results;
  }

 private:
  Server *server_;
};

}  // namespace lumin

#endif  // WINDOW_H_
