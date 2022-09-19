file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
add_executable(FlameEditor ${SRC_FILES})

set_custom_build_properties(FlameEditor)

# Defines "FL_DEBUG" for Debug build, and "FL_RELEASE" for Release build
set_build_config_macro_for_target(FlameEditor)

if(${FL_PROJ_GENERATOR} STREQUAL "Xcode")
    target_compile_definitions(Flameberry PRIVATE FL_XCODE_PROJ)
endif()

target_compile_definitions(FlameEditor PRIVATE ${FL_COMPILE_DEFINITIONS})

target_include_directories(FlameEditor PRIVATE ${FL_GRAPHICS_INCLUDE_DIRS} ${FL_SOURCE_DIR}/Flameberry/src/)
target_link_libraries(FlameEditor PRIVATE Flameberry ${FL_GRAPHICS_LIBS})