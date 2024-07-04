file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_executable(FlameberryEditor ${SRC_FILES})
target_compile_features(FlameberryEditor PUBLIC cxx_std_17)

set_custom_build_properties(FlameberryEditor)

# Defines "FBY_DEBUG" for Debug build, and "FBY_RELEASE" for Release build
set_build_config_macro_for_target(FlameberryEditor)

if(XCODE)
    target_compile_definitions(FlameberryEditor PRIVATE FBY_XCODE_PROJ)
    set_target_properties(FlameberryEditor PROPERTIES XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal")

    set_property(TARGET FlameberryEditor PROPERTY XCODE_SCHEME_ARGUMENTS "${FBY_SOURCE_DIR}/SandboxProject/SandboxProject.fbproj")

    # Set the environment variables required for vulkan, because Xcode doesn't seem to respect them
    # TODO: In the future when shipping the FlameberryEditor, it should be a good idea to have these baked in, rather than relying on the environment variables
    set_property(TARGET FlameberryEditor PROPERTY XCODE_SCHEME_ENVIRONMENT VK_ICD_FILENAMES=$ENV{VK_ICD_FILENAMES} VK_DRIVER_FILES=$ENV{VK_DRIVER_FILES} VK_ADD_LAYER_PATH=$ENV{VK_ADD_LAYER_PATH})
endif()

target_compile_definitions(FlameberryEditor PRIVATE ${FBY_COMPILE_DEFINITIONS})

target_include_directories(FlameberryEditor PRIVATE ${FBY_INCLUDE_DIRS} ${FBY_SOURCE_DIR}/Flameberry/Source/Developer ${FBY_SOURCE_DIR}/Flameberry/Source/Editor)
target_link_libraries(FlameberryEditor PRIVATE Flameberry ${FBY_LIBRARY_DEPENDENCIES})
