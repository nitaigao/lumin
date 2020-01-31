#include "key_bindings/key_binding_cmd.h"

#include <unistd.h>

void key_binding_cmd::run() {
  if (fork() == 0) {
    execl("/bin/sh", "/bin/sh", "-c", cmd.c_str(), NULL);
  }
}
