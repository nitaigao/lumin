#ifndef ISETTINGS_SERVICE_H_
#define ISETTINGS_SERVICE_H_

#include <map>
#include <memory>
#include <vector>
#include <string>

namespace lumin {

class Output;

struct DisplaySetting {
  int x;
  int y;
  int scale;
  bool enabled;
  bool primary;
};

typedef std::map<std::string, DisplaySetting> LayoutConfig;

class ISettingsService {
 public:
  virtual ~ISettingsService() { }
 public:
  virtual LayoutConfig display_find_layout(
    const std::vector<std::shared_ptr<Output>>& outputs) = 0;
};

}  // namespace lumin

#endif  // ISETTINGS_SERVICE_H_
