#include "controller.h"

int main(int argc, char *argv[]) {
  Controller server;
  server.run();
  server.destroy();

  return 0;
}
