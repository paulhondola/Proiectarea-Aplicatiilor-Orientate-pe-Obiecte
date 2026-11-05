add_library(image_lib STATIC
    Logger.cpp
)
target_include_directories(image_lib PUBLIC ${PROJECT_SOURCE_DIR}/inc)
target_link_libraries(image_lib PUBLIC stb_impl PRIVATE paoo_warnings)

add_executable(paoo
    main.cpp
)
target_link_libraries(paoo PRIVATE image_lib CLI11::CLI11 paoo_warnings)
