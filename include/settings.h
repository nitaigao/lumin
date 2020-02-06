#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <map>
#include <string>
#include <vector>

namespace lumin {

struct DisplaySetting {
  int x;
  int y;
  int scale;
  bool enabled;
  bool primary;
};

typedef std::map<std::string, DisplaySetting> LayoutConfig;

class Settings {
 public:
  LayoutConfig display_find_layout(const std::vector<std::string>& output_names);
};

}  // namespace lumin

#endif  // SETTINGS_H_
