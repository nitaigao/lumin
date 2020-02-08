#include "services/settings_service.h"

#include <dbus-c++-1/dbus-c++/dbus.h>

#include "settings_proxy.h"
#include "output.h"

namespace lumin {

class DisplayProxy : public org::os::Settings::Display_proxy,
  public DBus::IntrospectableProxy, public DBus::ObjectProxy {
 public:
  DisplayProxy(DBus::Connection &connection, const char *path, const char *name)
    : DBus::ObjectProxy(connection, path, name)
  { }
};

typedef DBus::Struct<std::string, int32_t, int32_t> FindLayoutArg;

LayoutConfig SettingsService::display_find_layout(
  const std::vector<std::shared_ptr<Output>>& outputs) {
  DBus::BusDispatcher dispatcher;
  DBus::default_dispatcher = &dispatcher;
  DBus::Connection bus = DBus::Connection::SystemBus();

  std::vector<FindLayoutArg> args;
  for (auto &output : outputs) {
    FindLayoutArg arg;
    arg._1 = output->id();
    arg._2 = output->width();
    arg._3 = output->height();
    args.push_back(arg);
  }

  DisplayProxy display(bus, "/org/os/Settings/Display", "org.os.Settings");
  auto results = display.FindLayout(args);

  LayoutConfig layout;

  for (auto &output : outputs) {
    auto result = results[output->id()];

    DisplaySetting display = {
      .x = result._1,
      .y = result._2,
      .scale = result._3,
      .enabled = result._4,
      .primary = result._5
    };

    layout[output->id()] = display;
  }

  return layout;
}

}  // namespace lumin
