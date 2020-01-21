#ifndef TURBO_OUTPUT_H
#define TURBO_OUTPUT_H

#include <wayland-server-core.h>

struct turbo_server;

struct turbo_output {
	wl_list link;
	turbo_server *server;
	struct wlr_output *wlr_output;
	wl_listener frame;
};

#endif
