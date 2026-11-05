cmake_minimum_required(VERSION 3.25)

project(PAOO CXX)

if(cxx_std_26 IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    set(CMAKE_CXX_STANDARD 26)
else()
    set(CMAKE_CXX_STANDARD 23)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(PAOO_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(PAOO_SANITIZE)
    # Before FetchContent, so libraries are instrumented as well.
    set(PAOO_SANITIZE_FLAGS -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer)
    add_compile_options(${PAOO_SANITIZE_FLAGS})
    add_link_options(${PAOO_SANITIZE_FLAGS})
endif()

include(FetchContent)
FetchContent_Declare(cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.7.2
    GIT_SHALLOW TRUE)
FetchContent_Declare(catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.16.1
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(cli11 catch2)

add_library(paoo_warnings INTERFACE)
target_compile_options(paoo_warnings INTERFACE -Wall -Wextra -Wpedantic)

enable_testing()

add_subdirectory(third_party)
add_subdirectory(src)
add_subdirectory(tests)
