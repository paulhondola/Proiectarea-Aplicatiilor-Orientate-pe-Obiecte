project(
  'PAOO',
  'cpp',
  version: '1.0.0',
  meson_version: '>= 0.56.0',
  default_options: [
    'cpp_std=c++14',
    'warning_level=3',
  ],
)

inc_dir = include_directories('inc')

sensor_lib = static_library(
  'sensor_lib',
  'src/SensorLogger.cpp',
  include_directories: inc_dir,
)

sensor_dep = declare_dependency(
  link_with: sensor_lib,
  include_directories: inc_dir,
)

lab_exe = executable(
  meson.project_name(),
  'src/main.cpp',
  dependencies: [sensor_dep],
)

test('sensor_logger_lifecycle', lab_exe)
