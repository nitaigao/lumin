project('turbo', 'cpp', default_options : [
  'warning_level=3',
  'werror=true',
  'cpp_std=c++2a',
  'c_std=c11'
])

add_global_arguments('-Wno-unused-parameter', language : 'cpp')

add_project_arguments([
  '-DWLR_USE_UNSTABLE'
], language: 'cpp')

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
  'src/wm_output.cpp',
  'src/wm_view_xwayland.cpp',
  'src/wm_view_xdg.cpp',
  'src/wm_view.cpp',
  'src/wm_server.cpp',
  'src/main.cpp',
]

executable(
  'turbo',
  sources,
  include_directories: includes,
  dependencies: dependencies
)
