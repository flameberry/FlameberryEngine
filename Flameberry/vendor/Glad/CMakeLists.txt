project(Glad VERSION 1.0)

file(GLOB_RECURSE SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
add_library(Glad STATIC ${SRC_FILES})

set_custom_build_properties(Glad)

target_include_directories(Glad PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")