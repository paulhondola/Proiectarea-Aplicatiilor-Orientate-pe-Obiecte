stb_inc = include_directories('stb', is_system: true)

stb_impl = static_library(
  'stb_impl',
  '../src/stb_impl.cpp',
  include_directories: stb_inc,
  override_options: ['warning_level=0'],
  cpp_args: ['-w'],
)

stb_dep = declare_dependency(link_with: stb_impl, include_directories: stb_inc)
