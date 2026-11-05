paoo_tests = executable(
  'paoo_tests',
  'test_libraries.cpp',
  'test_filters.cpp',
  'test_image.cpp',
  'test_image_io.cpp',
  'test_labeled_image.cpp',
  'test_logger.cpp',
  dependencies: [image_dep, catch2_dep],
)

test('paoo_tests', paoo_tests)
test('demo', paoo, args: ['demo'])
test(
  'filter_cli',
  find_program('test_cli.sh'),
  args: [paoo, files('data/sample.png'), meson.current_build_dir()],
)
