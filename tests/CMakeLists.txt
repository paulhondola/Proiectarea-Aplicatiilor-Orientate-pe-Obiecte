add_executable(paoo_tests
    test_libraries.cpp
    test_filters.cpp
    test_image.cpp
    test_image_io.cpp
    test_labeled_image.cpp
    test_logger.cpp
)
target_link_libraries(paoo_tests PRIVATE image_lib Catch2::Catch2WithMain paoo_warnings)

add_test(NAME paoo_tests COMMAND paoo_tests)
add_test(NAME demo COMMAND paoo demo)
add_test(NAME filter_cli
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test_cli.sh $<TARGET_FILE:paoo>
            ${CMAKE_CURRENT_SOURCE_DIR}/data/sample.png ${CMAKE_CURRENT_BINARY_DIR})
