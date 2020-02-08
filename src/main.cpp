#include "server.h"

int main(int argc, char *argv[]) {
  lumin::Server server;
  server.run();
  server.destroy();

  return 0;
}
