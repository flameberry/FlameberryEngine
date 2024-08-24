# ZLib
if(NOT ZLIB_FOUND)
	if(NOT TARGET zlibstatic)
		add_subdirectory(ZLib)
	endif()
	add_library(ZLIB::ZLIB ALIAS zlibstatic)
	get_target_property(ZLIB_INCLUDE_DIR zlibstatic INTERFACE_INCLUDE_DIRECTORIES)
	set(ZLIB_FOUND TRUE)
	set(ZLIB_LIBRARIES zlibstatic)

	# Assimp
	# set(ASSIMP_BUILD_ZLIB OFF)
endif()

if(WIN32)
	set(ZLIB_LIBRARY zlibstatic)
endif()

# LibPNG
add_subdirectory(LibPng)
add_library(PNG::PNG ALIAS png_static) # Needed for msdfgen find_package(PNG REQUIRED)

# This directory contains the pnglibconf.h file which is needed for compilation of msdfgen-ext
get_target_property(PNG_BINARY_DIR png_static BINARY_DIR)

if(WIN32)
	unset(ZLIB_LIBRARY)
endif()

# Freetype
add_subdirectory(Freetype)
add_library(Freetype::Freetype ALIAS freetype) # Needed for msdfgen find_package(Freetype REQUIRED)

# MSDF-Atlas-Gen
set(MSDF_ATLAS_BUILD_STANDALONE OFF)
set(MSDF_ATLAS_USE_VCPKG OFF)
set(MSDF_ATLAS_USE_SKIA OFF)
add_subdirectory(MsdfAtlasGen)

target_include_directories(msdfgen-ext PUBLIC ${PNG_BINARY_DIR})

