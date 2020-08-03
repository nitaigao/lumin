#ifndef POSIX_OS_H_
#define POSIX_OS_H_

#include <string>

#include "ios.h"

namespace lumin {

class PosixOS : public IOS {
 public:
  void set_env(const std::string& name, const std::string& value);
  std::string open_file(const std::string& filepath);
  bool file_exists(const std::string& filepath);
  void execute(const std::string& command);
};

}  // namespace lumin

#endif  // POSIX_OS_H_
