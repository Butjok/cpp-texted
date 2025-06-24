cmake_minimum_required(VERSION 3.26)
project(tracing)

set(CMAKE_CXX_STANDARD 20)

find_package(raylib REQUIRED)

add_executable(tracing main.cpp
        Widgets.h
        Widgets.cpp)

target_link_libraries(tracing raylib)
target_include_directories(tracing PRIVATE ${raylib_INCLUDE_DIRS})