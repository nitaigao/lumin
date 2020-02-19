#ifndef SHORTCUT_H_
#define SHORTCUT_H_

#include "compositor_adapter.h"
#include "server.h"

namespace lumin {

class CompositorEndpoint :
  public org::os::Compositor::Shortcut_adaptor,
  public org::os::Compositor::Window_adaptor,
  public DBus::IntrospectableAdaptor,
  public DBus::ObjectAdaptor
{
 public:
  CompositorEndpoint(DBus::Connection &connection, Server *server)
    : DBus::ObjectAdaptor(connection, "/org/os/Compositor")
    , server_(server) { }

  int Register(const int& code, const int& modifiers, const int& state) {
    return server_->add_keybinding(code, modifiers, state);
  }

  std::vector<std::string> Apps() {
    auto apps = server_->apps();
    auto results = std::vector<std::string>(apps.begin(), apps.end());
    return results;
  }

  void Focus(const std::string& app_id) {
    server_->focus_app(app_id);
  }

 private:
  Server *server_;
};

}  // namespace lumin

#endif  // SHORTCUT_H_
