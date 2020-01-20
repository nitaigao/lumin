#ifndef _OUTPUT_H
#define _OUTPUT_H

#include <time.h>
#include <wayland-server.h>
#include <vector>
#include <memory>

struct wlr_output;

class View;
class Server;

class Output {

public:

  Output(struct wlr_output *output, const Server* server);

public:

  void render(const std::vector<std::shared_ptr<View>>& views) const;

private:

  struct timespec last_frame;


public:

  struct wl_listener frame;

public:

  struct wlr_output *output;
  const Server *server;

};

#endif
