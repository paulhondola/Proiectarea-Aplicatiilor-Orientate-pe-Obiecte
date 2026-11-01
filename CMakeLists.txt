cmake_minimum_required(VERSION 3.10)

project(PAOO)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(sensor_lib STATIC
    src/SensorLogger.cpp
    inc/SensorLogger.hpp
)

target_include_directories(sensor_lib PUBLIC inc)

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC sensor_lib)
