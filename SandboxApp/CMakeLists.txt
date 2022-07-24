file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(SandboxApp ${SRC_FILES})

set_custom_build_properties(SandboxApp)

# Defines "FL_DEBUG" for Debug build, and "FL_RELEASE" for Release build
set_build_config_macro_for_target(SandboxApp)

if(${FL_PROJ_GENERATOR} STREQUAL "Xcode")
    target_compile_definitions(Flameberry PRIVATE FL_XCODE_PROJ)
endif()

target_compile_definitions(SandboxApp PRIVATE GLFW_INCLUDE_NONE FL_PROJECT_DIR="${FL_SOURCE_DIR}")

target_include_directories(SandboxApp PRIVATE ${FL_GRAPHICS_INCLUDE_DIRS} ${FL_SOURCE_DIR}/Flameberry/src/)
target_link_libraries(SandboxApp PRIVATE Flameberry ${FL_GRAPHICS_LIBS})