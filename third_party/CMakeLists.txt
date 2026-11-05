add_library(stb_impl STATIC ${PROJECT_SOURCE_DIR}/src/stb_impl.cpp)
target_include_directories(stb_impl SYSTEM PUBLIC stb)
target_compile_options(stb_impl PRIVATE -w)
