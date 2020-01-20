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
wayland_protocols = dependency('wayland-protocols', version: '>=1.17')

dependencies = [
  pixman,
  wlroots,
  wayland_server,
  xkbcommon,
  wayland_protocols
]

includes = include_directories('include')

sources = [
  'src/main.cpp',
]

executable(
  'turbo',
  sources,
  include_directories: includes,
  dependencies: dependencies
)
