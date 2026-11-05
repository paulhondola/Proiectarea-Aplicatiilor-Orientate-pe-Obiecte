project(
  'PAOO',
  'cpp',
  version: '1.0.0',
  meson_version: '>= 1.3.0',
  default_options: [
    'cpp_std=c++26,c++23',
    'warning_level=3',
  ],
)

cli11_dep = dependency('CLI11', default_options: ['warning_level=0'])
catch2_dep = dependency('catch2-with-main', default_options: ['warning_level=0'])

subdir('third_party')
subdir('src')
subdir('tests')
