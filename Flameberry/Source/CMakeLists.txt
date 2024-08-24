# Add Dependencies
add_subdirectory(ThirdParty/GLFW)
add_subdirectory(ThirdParty/YamlCpp)
add_subdirectory(ThirdParty/fmtlib)

set(INTERPROCEDURAL_OPTIMIZATION ON)
add_subdirectory(ThirdParty/JoltPhysics/Build)
SET_INTERPROCEDURAL_OPTIMIZATION()

# SPIRV-Reflect
set(SPIRV_REFLECT_EXECUTABLE OFF FORCE)
set(SPIRV_REFLECT_STATIC_LIB ON FORCE)
add_subdirectory(ThirdParty/SPIRV-Reflect)

# Add MsdfAtlasGen and it's dependencies
add_subdirectory(ThirdParty/MsdfAtlasGen)

if(WIN32)
    set(ASSIMP_WARNINGS_AS_ERRORS OFF)
endif()

set(ASSIMP_BUILD_ZLIB OFF)
add_subdirectory(ThirdParty/Assimp)

if(APPLE)
    # Assimp doesn't compile without this flag on Apple Silicon
    set_target_properties(assimp PROPERTIES COMPILE_FLAGS "-Wno-error=shorten-64-to-32")
endif()

# This is to ensure the include/assimp/config.h that is generated in the assimp build directory is found
get_target_property(ASSIMP_INCLUDE_DIRS assimp INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND FBY_INCLUDE_DIRS ${ASSIMP_INCLUDE_DIRS})

# Add Source Targets
add_subdirectory(Developer)
add_subdirectory(Editor)
