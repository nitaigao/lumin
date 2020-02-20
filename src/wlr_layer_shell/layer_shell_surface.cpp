#include "layer_shell.h"

#include <wlr-layer-shell.h>
#include <wayland-server.h>

#include <spdlog/spdlog.h>

void set_size(struct wl_client *client, struct wl_resource *resource,
  uint32_t width, uint32_t height)
{
  spdlog::debug("set_size");
}

void set_anchor(struct wl_client *client, struct wl_resource *resource, uint32_t anchor)
{
  spdlog::debug("set_anchor");
}

void set_exclusive_zone(struct wl_client *client, struct wl_resource *resource, int32_t zone)
{
  spdlog::debug("set_exclusive_zone");
}

void set_margin(struct wl_client *client, struct wl_resource *resource,
  int32_t top, int32_t right, int32_t bottom, int32_t left)
{
  spdlog::debug("set_margin");
}

void set_keyboard_interactivity(struct wl_client *client, struct wl_resource *resource,
  uint32_t keyboard_interactivity)
{
  spdlog::debug("set_keyboard_interactivity");
}

void get_popup(struct wl_client *client, struct wl_resource *resource, struct wl_resource *popup)
{
  spdlog::debug("get_popup");
}

void ack_configure(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{
  spdlog::debug("ack_configure");
}

void destroy(struct wl_client *client, struct wl_resource *resource)
{
  spdlog::debug("destroy");
}

void set_layer(struct wl_client *client, struct wl_resource *resource, uint32_t layer)
{
  spdlog::debug("set_layer");
}

static struct zwlr_layer_surface_v1_interface zwlr_layer_surface1_impl = {
  .set_size = set_size,
  .set_anchor = set_anchor,
  .set_exclusive_zone = set_exclusive_zone,
  .set_margin = set_margin,
  .set_keyboard_interactivity = set_keyboard_interactivity,
  .get_popup = get_popup,
  .ack_configure = ack_configure,
  .destroy = destroy,
  .set_layer = set_layer
};

void wlr_layer_surface_create(struct wl_client *client, uint32_t version,
  uint32_t id, void *data)
{
  wl_resource *resource = wl_resource_create(client, &zwlr_layer_surface_v1_interface, version, id);
  wl_resource_set_implementation(resource, &zwlr_layer_surface1_impl,
      data, NULL);
}
