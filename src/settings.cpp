#include "settings.h"

#include <iostream>
#include <string>
#include <vector>

#include "services/settings_service.h"

#include "output.h"

namespace lumin {

Settings::Settings()
{
  service_ = std::make_shared<SettingsService>();
}

LayoutConfig Settings::display_find_layout(
  const std::vector<std::shared_ptr<Output>>& outputs)
{
  auto results = service_->display_find_layout(outputs);
  return results;
}

}  // namespace lumin
