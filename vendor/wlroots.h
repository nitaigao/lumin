#ifndef WLROOTS_H_
#define WLROOTS_H_

#define static

extern "C" {
  #include <unistd.h>
  #include <wayland-contrib.h>
  #include <wayland-server-core.h>
  #include <wlr/backend.h>
  #include <wlr/backend/libinput.h>
  #include <wlr/render/wlr_renderer.h>
  #include <wlr/types/wlr_compositor.h>
  #include <wlr/types/wlr_cursor.h>
  #include <wlr/types/wlr_data_control_v1.h>
  #include <wlr/types/wlr_data_device.h>
  #include <wlr/types/wlr_data_device.h>
  #include <wlr/types/wlr_export_dmabuf_v1.h>
  #include <wlr/types/wlr_gtk_primary_selection.h>
  #include <wlr/types/wlr_layer_shell_v1.h>
  #include <wlr/types/wlr_input_device.h>
  #include <wlr/types/wlr_keyboard.h>
  #include <wlr/types/wlr_linux_dmabuf_v1.h>
  #include <wlr/types/wlr_matrix.h>
  #include <wlr/types/wlr_output_damage.h>
  #include <wlr/types/wlr_output_layout.h>
  #include <wlr/types/wlr_output.h>
  #include <wlr/types/wlr_pointer.h>
  #include <wlr/types/wlr_primary_selection_v1.h>
  #include <wlr/types/wlr_primary_selection.h>
  #include <wlr/types/wlr_region.h>
  #include <wlr/types/wlr_screencopy_v1.h>
  #include <wlr/types/wlr_seat.h>
  #include <wlr/types/wlr_xcursor_manager.h>
  #include <wlr/types/wlr_xdg_shell.h>
  #include <wlr/util/log.h>
  #include <wlr/util/region.h>
  #include <xkbcommon/xkbcommon.h>
}

#undef static

#endif  // WLROOTS_H_
