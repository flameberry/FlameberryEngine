file(GLOB_RECURSE SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm
)

if(MSVC OR XCODE)
    list(APPEND SRC_FILES ${GLSL_SOURCE_FILES})
endif()

if(WIN32)
    file(GLOB EXCLUDE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/MacOS/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/MacOS/*.h ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/MacOS/*.mm)
elseif(APPLE)
    file(GLOB EXCLUDE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Windows/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Windows/*.h)
endif()

exclude_from_list(SRC_FILES ${EXCLUDE_FILES})

add_library(Flameberry STATIC ${SRC_FILES} ${FBY_DEPENDENCY_SOURCE})
target_compile_features(Flameberry PUBLIC cxx_std_17)

add_dependencies(Flameberry CompileShadersTarget)

set_custom_build_properties(Flameberry)

# Defines "FBY_DEBUG" for Debug build, and "FBY_RELEASE" for Release build
set_build_config_macro_for_target(Flameberry)

if(XCODE)
    target_compile_definitions(Flameberry PRIVATE FBY_XCODE_PROJ)
endif()

target_include_directories(Flameberry PRIVATE ${FBY_INCLUDE_DIRS} ${FBY_SOURCE_DIR}/Flameberry/src)

# Add Dependencies
add_subdirectory(vendor/GLFW)
add_subdirectory(vendor/yaml-cpp)
add_subdirectory(vendor/fmtlib)

target_compile_definitions(Flameberry PRIVATE ${FBY_COMPILE_DEFINITIONS})

# "FBY_LIBRARY_DEPENDENCIES" defined in the {ROOT_DIRECTORY}/CMakeUtils/Dependencies.cmake
target_link_libraries(Flameberry PRIVATE ${FBY_LIBRARY_DEPENDENCIES})