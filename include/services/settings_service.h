#ifndef SETTINGS_SERVICE_H_
#define SETTINGS_SERVICE_H_

#include <memory>
#include <vector>

#include "services/isettings_service.h"

namespace lumin {

class SettingsService : public ISettingsService {
 public:
  LayoutConfig display_find_layout(const std::vector<std::shared_ptr<Output>>& outputs);
};

}  // namespace lumin

#endif  // SETTINGS_SERVICE_H_
