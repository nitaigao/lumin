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
  std::vector<std::shared_ptr<IOutput>> connected_outputs;
  std::copy_if(outputs.begin(), outputs.end(),
    std::back_inserter(connected_outputs), [](auto &output) {
    return output->connected();
  });

  std::map<std::string, OutputConfig> default_layout;

  bool primary = true;
  bool x = 0;
  for (auto output : connected_outputs) {
    OutputConfig config = {
      .scale = 1,
      .primary = primary,
      .enabled = true,
      .x = x,
      .y = 0,
    };
    auto name = output->id();
    default_layout[name] = config;
    primary = false;
    x += output->width();
  }

  const char *home = getenv("HOME");
  std::stringstream configFilePathStream;
  configFilePathStream << home << "/.config/monitors";
  std::string configFilePath = configFilePathStream.str();

  bool configFileExists = os_->file_exists(configFilePath);

  if (!configFileExists) {
    return default_layout;
  }

  auto configFileData = os_->open_file(configFilePath);
  auto document = YAML::Load(configFileData);

  for (auto node : document) {
    std::map<std::string, OutputConfig> layout;

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
        .primary = monitor["primary"].as<bool>(),
        .enabled = monitor["enabled"].as<bool>(),
        .x = monitor["x"].as<int>(),
        .y = monitor["y"].as<int>()
      };
      auto name = monitor["name"].as<std::string>();
      layout[name] = config;
    }

    return layout;
  }

  return default_layout;
}

}  // namespace lumin
