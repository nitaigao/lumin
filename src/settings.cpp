#include "settings.h"

#include <string>
#include <vector>

namespace lumin {

LayoutConfig Settings::display_find_layout(
  const std::vector<std::string>& output_names)
{
  LayoutConfig displays;

  DisplaySetting x11 = {
    .x = 0,
    .y = 0,
    .scale = 2,
    .enabled = true,
    .primary = true
  };

  displays["5502022416387535785"] = x11;

  DisplaySetting laptop = {
    .x = 320,
    .y = 1080,
    .scale = 3,
    .enabled = true,
    .primary = false
  };

  displays["13070870624578860264"] = laptop;

  DisplaySetting lg = {
    .x = 0,
    .y = 0,
    .scale = 2,
    .enabled = true,
    .primary = true
  };

  displays["10714521508828164035"] = lg;

  return displays;
}

}  // namespace lumin
