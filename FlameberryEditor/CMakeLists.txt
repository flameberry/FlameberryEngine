file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

add_executable(FlameberryEditor ${SRC_FILES})
target_compile_features(FlameberryEditor PUBLIC cxx_std_17)

set_custom_build_properties(FlameberryEditor)

# Defines "FBY_DEBUG" for Debug build, and "FBY_RELEASE" for Release build
set_build_config_macro_for_target(FlameberryEditor)

if(XCODE)
    target_compile_definitions(FlameberryEditor PRIVATE FBY_XCODE_PROJ)
endif()

set_property(TARGET FlameberryEditor PROPERTY XCODE_SCHEME_ARGUMENTS "${FBY_SOURCE_DIR}/SandboxProject/SandboxProject.fbproj")

target_compile_definitions(FlameberryEditor PRIVATE ${FBY_COMPILE_DEFINITIONS})

target_include_directories(FlameberryEditor PRIVATE ${FBY_INCLUDE_DIRS} ${FBY_SOURCE_DIR}/Flameberry/src/ ${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_link_libraries(FlameberryEditor PRIVATE Flameberry ${FBY_LIBRARY_DEPENDENCIES})
