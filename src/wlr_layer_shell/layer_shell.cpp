#include "layer_shell.h"

#include <spdlog/spdlog.h>

#include <wayland-server.h>
#include <wlr-layer-shell.h>

extern "C" {
#include <wlr/types/wlr_surface.h>
#include <wlr/types/wlr_xdg_shell.h>
}

static void get_layer_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id,
  struct wl_resource *surface_resource, struct wl_resource *output, uint32_t layer, const char *ns)
{
  // auto surface = wlr_surface_from_resource(surface_resource);
  // auto xdg_surface = wlr_xdg_surface_from_wlr_surface(surface);

  wlr_layer_surface_create(client, wl_resource_get_version(resource), id, NULL);

  spdlog::debug("get_layer_surface");
}

static struct zwlr_layer_shell_v1_interface layer_shell1_impl = {
  .get_layer_surface = get_layer_surface
};

static void layer_shell_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
  wl_resource *resource = wl_resource_create(client, &zwlr_layer_shell_v1_interface, version, id);
  wl_resource_set_implementation(resource, &layer_shell1_impl, data, NULL);
}

void wlr_layer_shell_create(struct wl_display* display)
{
  wl_global_create(display, &zwlr_layer_shell_v1_interface, 1, NULL, &layer_shell_bind);
}

