cmake_minimum_required(VERSION 3.10)

# Main Project
project(FlameberryEngine VERSION 1.0)

# C++17 or higher version of C++ Required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)

# Xcode scheme generation is helpful for things like passing command line arguments for development
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)

if(MSVC)
    add_compile_options(
        $<$<CONFIG:>:/MT>
        $<$<CONFIG:Debug>:/MTd>
        $<$<CONFIG:Release>:/MT>
    )
endif()

# Handle Generation for single config generators like MakeFiles, when no Config is specified explicitly in the command line
if(XCODE AND CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
    message(WARNING "CMAKE_BUILD_TYPE not specified, setting to Release as default configuration.")
endif()

set(FBY_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(FBY_PROJ_GENERATOR "${CMAKE_GENERATOR}")

# Include util functions and variables with paths to library dependencies
include(Flameberry/Build/CMake/Dependencies.cmake)
include(Flameberry/Build/CMake/CMakeUtils.cmake)

add_subdirectory(Flameberry/Shaders)
add_subdirectory(Flameberry/Source)