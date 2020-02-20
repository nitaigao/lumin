#ifndef LAYER_SHELL_H_
#define LAYER_SHELL_H_

struct wl_display;
struct wl_resource;

void wlr_layer_shell_create(struct wl_display* display);

void wlr_layer_surface_create(struct wl_client *client,
  unsigned int version, unsigned int id, void *data);

#endif  // LAYER_SHELL_H_
