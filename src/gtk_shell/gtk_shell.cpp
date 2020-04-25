#include "gtk_shell.h"

#include <gtk-shell.h>
#include <wayland-server.h>

#include <spdlog/spdlog.h>

extern "C" {
#include <wlr/types/wlr_surface.h>
#include <wlr/types/wlr_xdg_shell.h>
}

static void get_gtk_surface(struct wl_client *client, struct wl_resource *resource,
  unsigned int id, wl_resource* surface_resource)
{
  auto surface = wlr_surface_from_resource(surface_resource);
  auto xdg_surface = wlr_xdg_surface_from_wlr_surface(surface);

  gtk_surface_create(client, wl_resource_get_version(resource), id, xdg_surface);

  gtk_shell1_send_capabilities(resource, GTK_SHELL1_CAPABILITY_GLOBAL_APP_MENU);
  gtk_shell1_send_capabilities(resource, GTK_SHELL1_CAPABILITY_GLOBAL_MENU_BAR);
  gtk_shell1_send_capabilities(resource, GTK_SHELL1_CAPABILITY_DESKTOP_ICONS);

  spdlog::debug("get_gtk_surface");
}

static void set_startup_id(struct wl_client *client, struct wl_resource *resource,
  const char *startup_id)
{
  spdlog::debug("set_startup_id");
}

static void system_bell(struct wl_client *client, struct wl_resource *resource,
  struct wl_resource* surface)
{
  spdlog::debug("system_bell");
}

static void notify_launch(struct wl_client *client, struct wl_resource *resource,
  const char *startup_id)
{
  spdlog::debug("notify_launch");
}

static struct gtk_shell1_interface gtk_shell1_impl = {
  .get_gtk_surface = get_gtk_surface,
  .set_startup_id = set_startup_id,
  .system_bell = system_bell,
  .notify_launch = notify_launch
};

static void gtk_shell_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
  wl_resource *resource = wl_resource_create(client, &gtk_shell1_interface, version, id);
  wl_resource_set_implementation(resource, &gtk_shell1_impl, data, NULL);
}

void gtk_shell_create(struct wl_display* display)
{
  wl_global_create(display, &gtk_shell1_interface, 1, NULL, &gtk_shell_bind);
}
