cmake_minimum_required(VERSION 3.10)

# Main Project
project(FlameberryEngine VERSION 1.0)

# C++17 or higher version of C++ Required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)

# Handle Generation for single config generators like MakeFiles, when no Config is specified explicitly in the command line
if(XCODE AND CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
    message(WARNING "CMAKE_BUILD_TYPE not specified, setting to Release as default configuration.")
endif()

set(FBY_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(FBY_PROJ_GENERATOR "${CMAKE_GENERATOR}")

# Include util functions and variables with paths to library dependencies
include(CMake/Dependencies.cmake)
include(CMake/CMakeUtils.cmake)

# Get all shader files in shaders directory
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${FBY_SOURCE_DIR}/Flameberry/assets/shaders/vulkan/*.frag"
    "${FBY_SOURCE_DIR}/Flameberry/assets/shaders/vulkan/*.vert"
    "${FBY_SOURCE_DIR}/Flameberry/assets/shaders/vulkan/*.comp"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    message(STATUS "Adding ${FILE_NAME}.spv shader to compile target...")
    set(SPIRV "${FBY_SOURCE_DIR}/Flameberry/assets/shaders/vulkan/bin/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL}
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    CompileShadersTarget
    DEPENDS ${SPIRV_BINARY_FILES}
)

# Add the actual library directory and the Sandbox project
add_subdirectory(Flameberry)

# The Game Editor App
add_subdirectory(FlameberryEditor)