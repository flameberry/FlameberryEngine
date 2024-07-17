# Add Source Targets
add_subdirectory(Developer)
add_subdirectory(Editor)

# Add Dependencies
add_subdirectory(ThirdParty/GLFW)
add_subdirectory(ThirdParty/YamlCpp)
add_subdirectory(ThirdParty/fmtlib)

set(SPIRV_REFLECT_EXECUTABLE OFF)
set(SPIRV_REFLECT_STATIC_LIB ON)
add_subdirectory(ThirdParty/SPIRV-Reflect)

add_subdirectory(ThirdParty/Freetype)
add_library(Freetype::Freetype ALIAS freetype) # Needed for msdfgen find_package(Freetype REQUIRED)

set(MSDF_ATLAS_BUILD_STANDALONE OFF)
set(MSDF_ATLAS_USE_VCPKG OFF)
set(MSDF_ATLAS_USE_SKIA OFF)
add_subdirectory(ThirdParty/MsdfAtlasGen)
