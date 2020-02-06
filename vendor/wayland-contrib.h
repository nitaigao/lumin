#ifndef WAYLAND_CONTRIB_H_
#define WAYLAND_CONTRIB_H_

#define wl_list_first(pos, head, member)    \
  wl_container_of((head)->next, pos, member);

#endif  // WAYLAND_CONTRIB_H_
