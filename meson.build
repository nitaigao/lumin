project('lumin', 'cpp', default_options : [
  'warning_level=3',
  'werror=true',
  'cpp_std=c++2a',
  'c_std=c11'
])

add_global_arguments('-Wno-unused-parameter', language : 'cpp')
add_project_arguments(['-DWLR_USE_UNSTABLE'], language: 'cpp')

wayland_server = dependency('wayland-server')
wlroots = dependency('wlroots')
pixman = dependency('pixman-1')
xkbcommon = dependency('xkbcommon')
libinput = dependency('libinput')
wayland_protocols = dependency('wayland-protocols', version: '>=1.17')

dependencies = [
  pixman,
  wlroots,
  wayland_server,
  xkbcommon,
  wayland_protocols,
  libinput
]

includes = include_directories(['include', 'protocols'])

sources = [
  'src/key_binding.cpp',
  'src/key_bindings/key_binding_dock_right.cpp',
  'src/key_bindings/key_binding_dock_left.cpp',
  'src/key_bindings/key_binding_maximize.cpp',
  'src/key_bindings/key_binding_cmd.cpp',
  'src/key_bindings/key_binding_quit.cpp',
  'src/output.cpp',
  'src/views/xwayland_view.cpp',
  'src/views/xdg_view.cpp',
  'src/view.cpp',
  'src/controller.cpp',
  'src/main.cpp',
]

executable(
  'lumin',
  sources,
  include_directories: includes,
  dependencies: dependencies
)
