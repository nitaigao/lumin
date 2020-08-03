#include "posix_os.h"

#include <stdio.h>
#include <unistd.h>

#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

using std::filesystem::exists;
using std::filesystem::path;

namespace lumin {

void PosixOS::set_env(const std::string& name, const std::string& value)
{
  setenv(name.c_str(), value.c_str(), true);
}

std::string PosixOS::open_file(const std::string& filepath)
{
  std::ifstream ifs(filepath);
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  return buffer.str();
}

bool PosixOS::file_exists(const std::string& filepath)
{
  path pathToTest(filepath);
  return exists(pathToTest);
}

void PosixOS::execute(const std::string& command)
{
  if (fork() == 0) {
    execl("/bin/sh", "/bin/sh", "-c", command.c_str(), NULL);
  }
}

}  // namespace lumin
