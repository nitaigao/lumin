#ifndef IOS_H
#define IOS_H

#include <string>

namespace lumin {

class IOS {

 public:
  virtual ~IOS() {}

 public:
  virtual void set_env(const std::string& name, const std::string& value) = 0;

};

}  // namesapce lumin

#endif
