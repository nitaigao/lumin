#include "gtk_shell.h"

#include <gtk-shell.h>
#include <wayland-server-core.h>

#include "view.h"

#include <spdlog/spdlog.h>

extern "C" {
#include <wlr/types/wlr_surface.h>
#include <wlr/types/wlr_xdg_shell.h>
}

static void set_dbus_properties(struct wl_client *client, struct wl_resource *resource,
  const char *application_id,
  const char *app_menu_path,
  const char *menubar_path,
  const char *window_object_path,
  const char *application_object_path,
  const char *unique_bus_name)
{
  void *user_data = wl_resource_get_user_data(resource);
  auto xdg_surface = static_cast<wlr_xdg_surface*>(user_data);

  free(xdg_surface->toplevel->app_id);

  char *tmp = strdup(application_id);
  if (tmp == NULL) {
    return;
  }

  xdg_surface->toplevel->app_id = tmp;
  spdlog::error("set_dbus_properties application_id: {}", application_id);
}

static void set_modal(struct wl_client *client, struct wl_resource *resource)
{
  spdlog::debug("set_modal");
}

static void unset_modal(struct wl_client *client, struct wl_resource *resource)
{
  spdlog::debug("unset_modal");
}

static void present(struct wl_client *client, struct wl_resource *resource, uint32_t time)
{
  spdlog::debug("present");
}

static void request_focus(struct wl_client *client, struct wl_resource *resource,
  const char* startup_id)
{
  spdlog::debug("request_focus");
}

static struct gtk_surface1_interface gtk_surface1_impl = {
  .set_dbus_properties = set_dbus_properties,
  .set_modal = set_modal,
  .unset_modal = unset_modal,
  .present = present,
  .request_focus = request_focus
};

void gtk_surface_create(struct wl_client *client, uint32_t version, uint32_t id, void *data)
{
  wl_resource *resource = wl_resource_create(client, &gtk_surface1_interface, version, id);
  wl_resource_set_implementation(resource, &gtk_surface1_impl,
      data, NULL);
}

