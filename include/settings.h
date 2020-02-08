#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "services/isettings_service.h"

namespace lumin {

class Output;

class Settings {
 public:
  Settings();
 public:
  LayoutConfig display_find_layout(const std::vector<std::shared_ptr<Output>>& outputs);

 private:
  std::shared_ptr<ISettingsService> service_;
};

}  // namespace lumin

#endif  // SETTINGS_H_
