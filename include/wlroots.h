#ifndef WL_ROOTS_H_
#define WL_ROOTS_H_

#define static
// #define class char

extern "C" {
  #include <unistd.h>
  #include <wayland-server-core.h>
  #include <wayland-contrib.h>
  #include <wlr/backend.h>
  #include <wlr/render/wlr_renderer.h>
  #include <wlr/types/wlr_cursor.h>
  #include <wlr/types/wlr_compositor.h>
  #include <wlr/types/wlr_data_device.h>
  #include <wlr/types/wlr_input_device.h>
  #include <wlr/types/wlr_keyboard.h>
  #include <wlr/types/wlr_matrix.h>
  #include <wlr/types/wlr_output.h>
  #include <wlr/types/wlr_output_layout.h>
  #include <wlr/types/wlr_pointer.h>
  #include <wlr/types/wlr_seat.h>
  #include <wlr/types/wlr_xcursor_manager.h>
  #include <wlr/types/wlr_xdg_shell.h>
  #include <wlr/types/wlr_output_damage.h>
  #include <wlr/util/log.h>
  #include <wlr/util/region.h>
  #include <wlr/types/wlr_region.h>
  #include <wlr/backend/libinput.h>
  #include <xkbcommon/xkbcommon.h>
}

#undef static
// #undef class

#endif  // WL_ROOTS_H_