file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

add_executable(FlameberryEditor ${SRC_FILES})
target_compile_features(FlameberryEditor PUBLIC cxx_std_17)

set_custom_build_properties(FlameberryEditor)

# Defines "FL_DEBUG" for Debug build, and "FL_RELEASE" for Release build
set_build_config_macro_for_target(FlameberryEditor)

if(XCODE)
    target_compile_definitions(Flameberry PRIVATE FL_XCODE_PROJ)
endif()

target_compile_definitions(FlameberryEditor PRIVATE ${FL_COMPILE_DEFINITIONS})

target_include_directories(FlameberryEditor PRIVATE ${FL_INCLUDE_DIRS} ${FL_SOURCE_DIR}/Flameberry/src/ ${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_link_libraries(FlameberryEditor PRIVATE Flameberry ${FL_LIBRARY_DEPENDENCIES})