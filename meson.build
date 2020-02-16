project('lumin', 'cpp', default_options : [
  'warning_level=3',
  'werror=true',
  'cpp_std=c++2a',
  'c_std=c11'
])

add_global_arguments('-Wno-unused-parameter', language : 'cpp')
add_project_arguments(['-DWLR_USE_UNSTABLE'], language: 'cpp')

dbuscpp = dependency('dbus-c++-1')
gtk = dependency('gtk+-3.0')
libinput = dependency('libinput')
pixman = dependency('pixman-1')
spdlog = dependency('spdlog')
wayland_client = dependency('wayland-client')
wayland_protocols = dependency('wayland-protocols', version: '>=1.17')
wayland_server = dependency('wayland-server')
wlroots = dependency('wlroots')
xkbcommon = dependency('xkbcommon')

dependencies = [
  dbuscpp,
  gtk,
  libinput,
  pixman,
  spdlog,
  wayland_client,
  wayland_protocols,
  wayland_server,
  wlroots,
  xkbcommon,
]

includes = include_directories(['include', 'protocols', 'vendor', 'dbus'])

sources = [
  'src/cursor.cpp',
  'src/gtk_shell/gtk_shell.cpp',
  'src/gtk_shell/gtk_surface.cpp',
  'src/gtk_shell/gtk_shell_wl.cpp',
  'src/key_binding.cpp',
  'src/key_bindings/key_binding_cancel_activity.cpp',
  'src/key_bindings/key_binding_cmd.cpp',
  'src/key_bindings/key_binding_dock_left.cpp',
  'src/key_bindings/key_binding_dock_right.cpp',
  'src/key_bindings/key_binding_maximize.cpp',
  'src/key_bindings/key_binding_quit.cpp',
  'src/key_bindings/key_binding_switch_app.cpp',
  'src/key_bindings/key_binding_switch_app_reverse.cpp',
  'src/keyboard.cpp',
  'src/main.cpp',
  'src/output.cpp',
  'src/seat.cpp',
  'src/server.cpp',
  'src/services/settings_service.cpp',
  'src/settings.cpp',
  'src/shell.cpp',
  'src/switcher.cpp',
  'src/view.cpp',
]

executable(
  'lumin',
  sources,
  include_directories: includes,
  dependencies: dependencies
)
