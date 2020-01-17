#ifndef _OUTPUT_H
#define _OUTPUT_H

#include <time.h>
#include <wayland-server.h>

struct wlr_output;

class Output {

public:

  Output(struct wlr_output *output);

private:

  struct wlr_output *output;
  struct timespec last_frame;

};

#endif
