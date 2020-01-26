#include "wm_server.h"

int main(int argc, char *argv[]) {
  wm_server server;
  server.run();
  server.destroy();

  return 0;
}
