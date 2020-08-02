project('compositor', 'cpp', default_options : [
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
yamlcpp = dependency('yaml-cpp')

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
  yamlcpp
]

includes = include_directories(['include', 'protocols', 'vendor', 'dbus'])

sources = [
  'src/wlroots_platform.cpp',
  'src/posix_os.cpp',
  'src/display_config.cpp',
  'src/cursor.cpp',
  'src/gtk_shell/gtk_shell_wl.cpp',
  'src/gtk_shell/gtk_shell.cpp',
  'src/gtk_shell/gtk_surface.cpp',
  'src/key_binding.cpp',
  'src/keyboard.cpp',
  'src/xdg_shell_wl.cpp',
  'src/output.cpp',
  'src/seat.cpp',
  'src/server.cpp',
  'src/services/settings_service.cpp',
  'src/settings.cpp',
  'src/view.cpp',
  'src/xdg_view.cpp',
]

compositor = shared_library(
  'compositor',
  sources,
  include_directories: includes,
  dependencies: dependencies
)

executable(
  'compositor',
  'src/main.cpp',
  include_directories: includes,
  link_with: compositor,
  dependencies: dependencies
)

gtest = dependency('gtest')
gmock = dependency('gmock')

tests_dependencies = [
  gtest,
  gmock
]

tests_sources = [
  'tests/server_tests.cpp',
  'tests/display_config_tests.cpp',
  'tests/main.cpp'
]

executable(
  'tests',
  tests_sources,
  include_directories: ['tests', includes],
  dependencies: [dependencies, tests_dependencies],
  link_with: compositor
)
