#ifndef SHORTCUT_H_
#define SHORTCUT_H_

#include "shortcut_adapter.h"
#include "server.h"
#include <spdlog/spdlog.h>

namespace lumin {

class ShortcutEndpoint : public org::os::Compositor::Shortcut_adaptor,
  public DBus::IntrospectableAdaptor,
  public DBus::ObjectAdaptor
{
 public:
  ShortcutEndpoint(DBus::Connection &connection, Server *server)
    : DBus::ObjectAdaptor(connection, "/org/os/Compositor")
    , server_(server) { }

  int Register(const int& code, const int& modifiers, const int& state) {
    return server_->add_keybinding(code, modifiers, state);
  }

 private:
  Server *server_;
};

}  // namespace lumin

#endif  // SHORTCUT_H_
