#include "turbo_server.h"

int main(int argc, char *argv[]) {
  turbo_server server;
  server.run();
  server.destroy();

  return 0;
}
