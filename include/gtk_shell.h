#ifndef GTK_SHELL_H_
#define GTK_SHELL_H_

struct wl_display;
struct wl_resource;

void gtk_shell_create(wl_display *display);

void gtk_surface_create(struct wl_client *client,
  unsigned int version, unsigned int id, void *data);

#endif  // GTK_SHELL_H_
