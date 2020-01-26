#include "wm_key_binding_cmd.h"

#include <unistd.h>

void wm_key_binding_cmd::run() {
  if (fork() == 0) {
    execl("/bin/sh", "/bin/sh", "-c", cmd.c_str(), (void *)NULL);
  }
}
