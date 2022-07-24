cmake_minimum_required(VERSION 3.10)

project(FlameberryEngine VERSION 1.0)

# include(CMake/Dependencies.cmake)
# include(CMake/CMakeUtils.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)

set(FL_SOURCE_DIR ${CMAKE_SOURCE_DIR})

add_subdirectory(Flameberry)
add_subdirectory(SandboxApp)
