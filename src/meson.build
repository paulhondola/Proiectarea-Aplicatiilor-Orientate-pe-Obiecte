inc_dir = include_directories('../inc')

image_lib = static_library(
  'image_lib',
  'Image.cpp',
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
  'demos/lab3.cpp',
  'demos/lab4.cpp',
  'demos/lab4_order_a.cpp',
  'demos/lab4_order_b.cpp',
  dependencies: [image_dep, cli11_dep],
)
