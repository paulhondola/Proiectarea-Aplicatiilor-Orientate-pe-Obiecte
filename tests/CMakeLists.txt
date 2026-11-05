add_executable(paoo_tests
    test_libraries.cpp
    test_logger.cpp
)
target_link_libraries(paoo_tests PRIVATE image_lib Catch2::Catch2WithMain paoo_warnings)

add_test(NAME paoo_tests COMMAND paoo_tests)
