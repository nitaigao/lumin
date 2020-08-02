#ifndef IDISPLAY_CONFIG_H_
#define IDISPLAY_CONFIG_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace lumin {

class IOutput;

struct OutputConfig {
  int scale;
  bool primary;
};

class IDisplayConfig {
 public:
  virtual ~IDisplayConfig() {}

 public:
  virtual std::map<std::string, OutputConfig> find_layout(
    const std::vector<std::shared_ptr<IOutput>>& outputs) = 0;
};

}  // namespace lumin

#endif  // IDISPLAY_CONFIG_H_
