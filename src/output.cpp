#include "output.h"

#include <time.h>

extern "C" {
  #include <wlr/backend.h>
  #include <wlr/interfaces/wlr_output.h>
}

Output::Output(struct wlr_output *output_)
  : output(output_) {
    clock_gettime(CLOCK_MONOTONIC, &last_frame);
  output->data = this;
}
