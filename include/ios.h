#ifndef IOS_H_
#define IOS_H_

#include <string>

namespace lumin {

class IOS {
 public:
  virtual ~IOS() {}

 public:
  virtual void set_env(const std::string& name, const std::string& value) = 0;
  virtual std::string open_file(const std::string& filepath) = 0;
  virtual void execute(const std::string& command) = 0;
};

}  // namespace lumin

#endif  // IOS_H_
