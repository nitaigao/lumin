#include "layer_shell.h"

#include <spdlog/spdlog.h>

#include <wayland-server.h>
#include <wlr-layer-shell.h>

extern "C" {
#include <wlr/types/wlr_surface.h>
#include <wlr/types/wlr_xdg_shell.h>
}

#include "server.h"
#include "view.h"

using namespace lumin;

static void get_layer_surface(wl_client *client, wl_resource *resource, uint32_t id,
  wl_resource *surface_resource, wl_resource *output, uint32_t layer, const char *ns)
{
  // auto surface = wlr_surface_from_resource(surface_resource);
  // auto user_data = wl_resource_get_user_data(resource);

  // auto server = static_cast<Server*>(user_data);
  // auto view = server->view_from_surface(surface);

  spdlog::debug("layer = {}", layer);

  // view->layer = static_cast<ViewLayer>(layer);

  wlr_layer_surface_create(client, wl_resource_get_version(resource), id, user_data);
}

static struct zwlr_layer_shell_v1_interface layer_shell1_impl = {
  .get_layer_surface = get_layer_surface
};

static void layer_shell_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
  wl_resource *resource = wl_resource_create(client, &zwlr_layer_shell_v1_interface, version, id);
  wl_resource_set_implementation(resource, &layer_shell1_impl, data, NULL);
}

void wlr_layer_shell_create(struct wl_display* display, lumin::Server *server)
{
  wl_global_create(display, &zwlr_layer_shell_v1_interface, 2, server, &layer_shell_bind);
}

