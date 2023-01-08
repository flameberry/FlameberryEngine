file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm)

if(FL_GRAPHICS_API STREQUAL "OpenGL")
    file(GLOB EXCLUDE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Vulkan/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Vulkan/*.h)
elseif(FL_GRAPHICS_API STREQUAL "Vulkan")
    file(GLOB EXCLUDE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/OpenGL/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/OpenGL/*.h)
endif()

exclude_from_list(SRC_FILES ${EXCLUDE_FILES})

add_library(Flameberry STATIC ${SRC_FILES} ${IMGUI_SRC} ${IMGUIZMO_SRC})
target_compile_features(Flameberry PUBLIC cxx_std_17)

set_custom_build_properties(Flameberry)

# Defines "FL_DEBUG" for Debug build, and "FL_RELEASE" for Release build
set_build_config_macro_for_target(Flameberry)

if(${FL_PROJ_GENERATOR} STREQUAL "Xcode")
    target_compile_definitions(Flameberry PRIVATE FL_XCODE_PROJ)
endif()

target_include_directories(Flameberry PRIVATE ${FL_GRAPHICS_INCLUDE_DIRS} ${FL_SOURCE_DIR}/Flameberry/src)
target_include_directories(Flameberry PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/Glad/include)

# Add Dependencies
add_subdirectory(vendor/GLFW)
add_subdirectory(vendor/Glad)

target_compile_definitions(Flameberry PRIVATE ${FL_COMPILE_DEFINITIONS})

# "FL_GRAPHICS_LIBS" defined in the {ROOT_DIRECTORY}/CMakeUtils/Dependencies.cmake
target_link_libraries(Flameberry PRIVATE ${FL_GRAPHICS_LIBS})