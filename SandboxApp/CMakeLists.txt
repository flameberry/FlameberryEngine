file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(SandboxApp ${SRC_FILES})

set_custom_build_properties(SandboxApp)

target_include_directories(SandboxApp PRIVATE ${FL_SOURCE_DIR}/Flameberry/src/)
target_link_libraries(SandboxApp PRIVATE Flameberry)