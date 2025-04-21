find_package(ZLIB REQUIRED)

add_subdirectory(LibPng)
add_library(PNG::PNG ALIAS png_static) # Needed for msdfgen find_package(PNG REQUIRED)

# This directory contains the pnglibconf.h file which is needed for compilation of msdfgen-ext
get_target_property(PNG_BINARY_DIR png_static BINARY_DIR)

# Freetype
add_subdirectory(Freetype)
add_library(Freetype::Freetype ALIAS freetype) # Needed for msdfgen find_package(Freetype REQUIRED)

# MSDF-Atlas-Gen
set(MSDF_ATLAS_BUILD_STANDALONE OFF)
set(MSDF_ATLAS_USE_VCPKG OFF)
set(MSDF_ATLAS_USE_SKIA OFF)
add_subdirectory(MsdfAtlasGen)

target_include_directories(msdfgen-ext PUBLIC ${PNG_BINARY_DIR})
