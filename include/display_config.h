#ifndef DISPLAY_CONFIG_H_
#define DISPLAY_CONFIG_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "idisplay_config.h"

namespace lumin {

class IOS;

class DisplayConfig : public IDisplayConfig {
 public:
  DisplayConfig(const std::shared_ptr<IOS>& os);

 public:
  std::map<std::string, OutputConfig> find_layout(
    const std::vector<std::shared_ptr<IOutput>>& outputs);

 private:
  std::shared_ptr<IOS> os_;
};

}  // namespace lumin

#endif  // DISPLAY_CONFIG_H_
