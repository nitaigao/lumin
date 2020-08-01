#ifndef POSIX_OS_H
#define POSIX_OS_H

#include "ios.h"

namespace lumin {

class PosixOS : public IOS {

public:
  void set_env(const std::string& name, const std::string& value);

};

} // namespace lumin

#endif
