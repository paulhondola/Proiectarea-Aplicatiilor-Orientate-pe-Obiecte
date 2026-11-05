inc_dir = include_directories('../inc')

image_lib = static_library(
  'image_lib',
  'Logger.cpp',
  include_directories: inc_dir,
  dependencies: [stb_dep],
)

image_dep = declare_dependency(
  link_with: image_lib,
  include_directories: inc_dir,
  dependencies: [stb_dep],
)

paoo = executable(
  'paoo',
  'main.cpp',
  dependencies: [image_dep, cli11_dep],
)
