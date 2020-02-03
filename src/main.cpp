#include "server.h"

int main(int argc, char *argv[]) {
  Server server;
  server.run();
  server.destroy();

  return 0;
}
