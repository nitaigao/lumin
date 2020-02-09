project('lumin', 'cpp', default_options : [
  'warning_level=3',
  'werror=true',
  'cpp_std=c++2a',
  'c_std=c11'
])

add_global_arguments('-Wno-unused-parameter', language : 'cpp')
add_project_arguments(['-DWLR_USE_UNSTABLE'], language: 'cpp')

dbuscpp = dependency('dbus-c++-1')
libinput = dependency('libinput')
pixman = dependency('pixman-1')
spdlog = dependency('spdlog')
wayland_protocols = dependency('wayland-protocols', version: '>=1.17')
wayland_server = dependency('wayland-server')
wlroots = dependency('wlroots')
xkbcommon = dependency('xkbcommon')

dependencies = [
  dbuscpp,
  libinput,
  spdlog,
  pixman,
  wayland_protocols,
  wayland_server,
  wlroots,
  xkbcommon,
]

includes = include_directories(['include', 'protocols', 'vendor', 'dbus'])

sources = [
  'src/cursor.cpp',
  'src/key_binding.cpp',
  'src/key_bindings/key_binding_cmd.cpp',
  'src/key_bindings/key_binding_dock_left.cpp',
  'src/key_bindings/key_binding_dock_right.cpp',
  'src/key_bindings/key_binding_maximize.cpp',
  'src/key_bindings/key_binding_quit.cpp',
  'src/keyboard.cpp',
  'src/main.cpp',
  'src/output.cpp',
  'src/seat.cpp',
  'src/server.cpp',
  'src/services/settings_service.cpp',
  'src/settings.cpp',
  'src/view.cpp',
]

executable(
  'lumin',
  sources,
  include_directories: includes,
  dependencies: dependencies
)
