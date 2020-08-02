#include "display_config.h"

#include <yaml-cpp/yaml.h>

#include "idisplay_config.h"
#include "output.h"

namespace lumin {

std::map<std::string, OutputConfig> DisplayConfig::find_layout(
  const std::vector<std::shared_ptr<IOutput>>& outputs)
{
  std::map<std::string, OutputConfig> layout;
  auto document = YAML::LoadFile("/home/nk/.config/monitors");

  for (auto node : document) {
    if (node.size() != outputs.size()) {
      continue;
    }

    bool not_found = false;
    for (auto monitor : node) {
      auto name = monitor["name"].as<std::string>();
      auto condition = [name](auto &el) { return el->id().compare(name) == 0; };
      auto result = std::find_if(outputs.begin(), outputs.end(), condition);
      if (result == outputs.end()) {
        not_found = true;
        break;
      }
    }

    if (not_found){
      continue;
    }

    for (auto monitor : node) {
      OutputConfig config = {
        .scale = monitor["scale"].as<int>(),
        .primary = monitor["primary"].as<bool>()
      };
      auto name = monitor["name"].as<std::string>();
      layout[name] = config;
    }

    return layout;
  }

  return layout;
}

}  // namespace lumin
