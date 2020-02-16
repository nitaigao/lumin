#include "services/settings_service.h"

#include <dbus-c++-1/dbus-c++/dbus.h>
#include <spdlog/spdlog.h>

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

void log_outputs(const std::vector<std::shared_ptr<Output>>& outputs) {
  std::stringstream message;

  for (auto &output : outputs) {
    if (output != (*outputs.begin())) {
      message << ", ";
    }
    message << output->id();
  }

  spdlog::debug("Fetch layout: {}", message.str());
}

LayoutConfig SettingsService::display_find_layout(
  const std::vector<std::shared_ptr<Output>>& outputs) {
  LayoutConfig layout;
  if (outputs.empty()) {
    return layout;
  }

  log_outputs(outputs);

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
