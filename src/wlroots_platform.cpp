#include "wlroots_platform.h"

#include <wlroots.h>
#include <iostream>
#include <spdlog/spdlog.h>

#include "cursor.h"
#include "keyboard.h"
#include "seat.h"
#include "output.h"
#include "xdg_view.h"
#include "gtk_shell.h"

namespace lumin {

WlRootsPlatform::WlRootsPlatform()
{

}

WlRootsPlatform::WlRootsPlatform(const std::shared_ptr<ICursor>& cursor)
  : cursor_(cursor)
{

}

bool WlRootsPlatform::init()
{
  //  Disable atomic for smooth cursor
  setenv("WLR_DRM_NO_ATOMIC", "1", true);
  // Modifiers can stop hotplugging working correctly
  setenv("WLR_DRM_NO_MODIFIERS", "1", true);

  wlr_log_init(WLR_INFO, NULL);

  display_ = wl_display_create();

  if (!display_) {
    spdlog::error("Failed to create wayland display");
    return false;
  }

  backend_ = wlr_backend_autocreate(display_, NULL);

  if (!backend_) {
    spdlog::error("Failed to create wlr backend");
    return false;
  }

  renderer_ = wlr_backend_get_renderer(backend_);

  if (!renderer_) {
    spdlog::error("Failed to fetch wlr renderer");
    return false;
  }

  if (!wlr_renderer_init_wl_display(renderer_, display_)) {
    spdlog::error("Failed to init wlr renderer with wl_display");
    return false;
  }

  auto compositor = wlr_compositor_create(display_, renderer_);

  if (!compositor) {
    spdlog::error("Failed to create wlr compositor");
    return false;
  }

  auto data_device_manager = wlr_data_device_manager_create(display_);

  if (!data_device_manager) {
    spdlog::error("Failed to create data device manager");
    return false;
  }

  layout_ = wlr_output_layout_create();

  if (!layout_) {
    spdlog::error("Failed to create wlr layout");
    return false;
  }

  xdg_shell_ = wlr_xdg_shell_create(display_);

  if (!xdg_shell_) {
    spdlog::error("Failed to create wlr xdg shell");
    return false;
  }

  xcursor_manager_ = wlr_xcursor_manager_create("default", 24);

  if (!xcursor_manager_) {
    spdlog::error("Failed to create wlr xcursor manager");
    return false;
  }

  auto data_control_manager = wlr_data_control_manager_v1_create(display_);

  if (!data_control_manager) {
    spdlog::error("Failed to create wlr data control manager");
    return false;
  }

  auto primary_selection_device_manager = wlr_primary_selection_v1_device_manager_create(display_);

  if (!primary_selection_device_manager) {
    spdlog::error("Failed to create wlr primary selection device manager");
    return false;
  }

  output_manager_ = wlr_output_manager_v1_create(display_);

  if (!output_manager_) {
    spdlog::error("Failed to create wlr output manager");
    return false;
  }

  auto xdg_output_manager = wlr_xdg_output_manager_v1_create(display_, layout_);

  if (!xdg_output_manager) {
    spdlog::error("Failed to create wlr xdg output manager");
    return false;
  }

  auto screencopy_manager = wlr_screencopy_manager_v1_create(display_);

  if (!screencopy_manager) {
    spdlog::error("Failed to create wlr screenscopy manager");
    return false;
  }

  gtk_shell_create(display_);

  wlr_seat *seat = wlr_seat_create(display_, "seat0");

  if (!seat) {
    spdlog::error("Failed to create wlr seat");
    return false;
  }


  seat_ = std::make_shared<Seat>(seat);
  cursor_ = std::make_shared<Cursor>(layout_, seat_.get());

  seat_->set_pointer(cursor_);

  const char *socket = wl_display_add_socket_auto(display_);
  if (!socket) {
    spdlog::error("Failed to wayland socket");
    wlr_backend_destroy(backend_);
    return false;
  }

  setenv("WAYLAND_DISPLAY", socket, true);
  spdlog::info("WAYLAND_DISPLAY={}", socket);

  spdlog::debug("Platform initialized");

  return true;
}

bool WlRootsPlatform::start()
{
  new_output.notify = new_output_notify;
  wl_signal_add(&backend_->events.new_output, &new_output);

  new_surface.notify = new_surface_notify;
  wl_signal_add(&xdg_shell_->events.new_surface, &new_surface);

  new_input.notify = new_input_notify;
  wl_signal_add(&backend_->events.new_input, &new_input);

  if (!wlr_backend_start(backend_)) {
    spdlog::error("Failed to start wlr backend");
    wlr_backend_destroy(backend_);
    wl_display_destroy(display_);
    return false;
  }

  spdlog::debug("Platform started");

  return true;
}

void WlRootsPlatform::new_output_notify(wl_listener *listener, void *data)
{
  WlRootsPlatform *platform = wl_container_of(listener, platform, new_output);
  auto wlr_output = static_cast<struct wlr_output*>(data);

  auto damage = wlr_output_damage_create(wlr_output);
  auto output = std::make_shared<Output>(wlr_output,
    platform->renderer_, damage, platform->layout_);

  wlr_output->data = output.get();

  output->init();

  platform->on_new_output.emit(output);
}

std::shared_ptr<ICursor> WlRootsPlatform::cursor() const
{
  return cursor_;
}

void WlRootsPlatform::new_surface_notify(wl_listener *listener, void *data)
{
  auto xdg_surface = static_cast<wlr_xdg_surface*>(data);
  if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
    return;
  }

  WlRootsPlatform *platform = wl_container_of(listener, platform, new_surface);

  std::shared_ptr<View> view = std::make_shared<XDGView>(xdg_surface,
    platform->cursor_.get(), platform->layout_, platform->seat_.get());

  platform->on_new_view.emit(view);
}

void WlRootsPlatform::new_keyboard(wlr_input_device *device)
{
  auto keyboard = std::make_shared<Keyboard>(device, seat_.get());
  keyboard->setup();
  on_new_keyboard.emit(keyboard);
}

void WlRootsPlatform::new_input_notify(wl_listener *listener, void *data)
{
  WlRootsPlatform *platform = wl_container_of(listener, platform, new_input);
  auto device = static_cast<struct wlr_input_device *>(data);

  switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD: {
      platform->new_keyboard(device);
      break;
    }
    case WLR_INPUT_DEVICE_POINTER:
      platform->cursor_->add_device(device);
      break;
    case WLR_INPUT_DEVICE_SWITCH:
      platform->new_switch(device);
      break;
    default:
      break;
  }
}

void WlRootsPlatform::toggle_lid_notify(wl_listener *listener, void *data)
{
  auto event = static_cast<wlr_event_switch_toggle *>(data);

  if (event->switch_type != WLR_SWITCH_TYPE_LID) {
    return;
  }

  WlRootsPlatform *platform = wl_container_of(listener, platform, lid_toggle);
  bool enabled = event->switch_state == WLR_SWITCH_STATE_OFF;
  platform->on_lid_switch.emit(enabled);
}

void WlRootsPlatform::new_switch(wlr_input_device *device)
{
  spdlog::debug("new_switch");
  lid_toggle.notify = toggle_lid_notify;
  wl_signal_add(&device->switch_device->events.toggle, &lid_toggle);
}

void WlRootsPlatform::run()
{
  spdlog::debug("Running platform");

  wl_display_run(display_);
}

void WlRootsPlatform::destroy()
{
  wl_display_destroy_clients(display_);
  wlr_xcursor_manager_destroy(xcursor_manager_);
  wlr_backend_destroy(backend_);
  wlr_output_layout_destroy(layout_);
  wl_display_destroy(display_);
}

void WlRootsPlatform::terminate()
{
  wl_display_terminate(display_);
}

void WlRootsPlatform::add_idle(idle_func func, void* data)
{
  auto *event_loop = wl_display_get_event_loop(display_);
  wl_event_loop_add_idle(event_loop, func, data);
}

std::shared_ptr<Seat> WlRootsPlatform::seat() const
{
  return seat_;
}

Output* WlRootsPlatform::output_at(int x, int y) const
{
  wlr_output* wlr_output = wlr_output_layout_output_at(layout_, x, y);

  if (wlr_output == nullptr) {
    return nullptr;
  }

  Output *output = static_cast<Output*>(wlr_output->data);
  return output;
}

}  // namespace lumin
