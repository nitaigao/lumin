#include "display_config.h"

#include <yaml-cpp/yaml.h>

#include "idisplay_config.h"
#include "output.h"
#include "ios.h"

namespace lumin {

DisplayConfig::DisplayConfig(const std::shared_ptr<IOS>& os)
  : os_(os)
{

}

std::map<std::string, OutputConfig> DisplayConfig::find_layout(
  const std::vector<std::shared_ptr<IOutput>>& outputs)
{
  std::map<std::string, OutputConfig> layout;
  auto configFileData = os_->open_file("/home/nk/.config/monitors");
  auto document = YAML::Load(configFileData);

  std::vector<std::shared_ptr<IOutput>> connected_outputs;
  std::copy_if(outputs.begin(), outputs.end(),
    std::back_inserter(connected_outputs), [](auto &output) {
    return output->connected();
  });

  for (auto node : document) {
    auto monitor_count = node.size();
    if (monitor_count != connected_outputs.size()) {
      continue;
    }

    bool not_found = false;
    for (auto monitor : node) {
      auto name = monitor["name"].as<std::string>();
      auto condition = [name](auto &el) { return el->id().compare(name) == 0; };
      auto result = std::find_if(connected_outputs.begin(), connected_outputs.end(), condition);
      if (result == connected_outputs.end()) {
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
