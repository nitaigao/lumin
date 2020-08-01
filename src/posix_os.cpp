#include "posix_os.h"

namespace lumin {

void PosixOS::set_env(const std::string& name, const std::string& value)
{
  setenv(name.c_str(), value.c_str(), true);
}

} // namespace lumin
