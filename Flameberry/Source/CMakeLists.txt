# Add Dependencies
add_subdirectory(ThirdParty/GLFW)
add_subdirectory(ThirdParty/YamlCpp)
add_subdirectory(ThirdParty/fmtlib)

# Assimp
add_subdirectory(ThirdParty/Assimp)

# Assimp doesn't compile without this flag on Apple Silicon
set_target_properties(assimp PROPERTIES COMPILE_FLAGS "-Wno-error=shorten-64-to-32")

# This is to ensure the include/assimp/config.h that is generated in the assimp build directory is found
get_target_property(ASSIMP_INCLUDE_DIRS assimp INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND FBY_INCLUDE_DIRS ${ASSIMP_INCLUDE_DIRS})

# SPIRV-Reflect
set(SPIRV_REFLECT_EXECUTABLE OFF)
set(SPIRV_REFLECT_STATIC_LIB ON)
add_subdirectory(ThirdParty/SPIRV-Reflect)

add_subdirectory(ThirdParty/Freetype)
add_library(Freetype::Freetype ALIAS freetype) # Needed for msdfgen find_package(Freetype REQUIRED)

set(MSDF_ATLAS_BUILD_STANDALONE OFF)
set(MSDF_ATLAS_USE_VCPKG OFF)
set(MSDF_ATLAS_USE_SKIA OFF)
add_subdirectory(ThirdParty/MsdfAtlasGen)

set(INTERPROCEDURAL_OPTIMIZATION ON)
add_subdirectory(ThirdParty/JoltPhysics/Build)
SET_INTERPROCEDURAL_OPTIMIZATION()

# Add Source Targets
add_subdirectory(Developer)
add_subdirectory(Editor)