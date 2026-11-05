paoo_tests = executable(
  'paoo_tests',
  'test_libraries.cpp',
  'test_image.cpp',
  'test_logger.cpp',
  dependencies: [image_dep, catch2_dep],
)

test('paoo_tests', paoo_tests)
test('demo', paoo, args: ['demo'])
