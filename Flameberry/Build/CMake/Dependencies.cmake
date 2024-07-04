# Set Global Variables
set(FBY_GRAPHICS_API "Vulkan")
set(FBY_LIBRARY_DEPENDENCIES)
set(FBY_INCLUDE_DIRS)

# Vulkan Helper Libs
find_package(Vulkan)

IF(UNIX AND NOT APPLE)
    set(LINUX TRUE)
ENDIF()

IF(WIN32)
    IF(NOT Vulkan_FOUND)
        # If 64 bit compiler then use 64 bit version of vulkan library
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(VULKAN_LIB_PATH "${FBY_SOURCE_DIR}/Flameberry/Binaries/ThirdParty/Vulkan/x64")
        else()
            set(VULKAN_LIB_PATH "${FBY_SOURCE_DIR}/Flameberry/Binaries/ThirdParty/Vulkan/x86")
        endif()

        find_library(Vulkan_LIBRARY NAMES vulkan-1 PATHS ${VULKAN_LIB_PATH} REQUIRED)

        IF(Vulkan_LIBRARY)
            set(Vulkan_FOUND ON)
            MESSAGE(STATUS "Using bundled Vulkan library version")
        ENDIF()
    ENDIF()
ELSEIF(LINUX)
    IF(NOT Vulkan_FOUND)
        find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${FBY_SOURCE_DIR}/Flameberry/Binaries/ThirdParty/Vulkan/Linux" REQUIRED)

        IF(Vulkan_LIBRARY)
            set(Vulkan_FOUND ON)
            MESSAGE(STATUS "Using bundled Vulkan library version")
        ENDIF()
    ENDIF()
ELSEIF(APPLE)
    IF(NOT Vulkan_FOUND)
        find_library(Vulkan_LIBRARY NAMES vulkan HINTS "${FBY_SOURCE_DIR}/Flameberry/Binaries/ThirdParty/Vulkan/MacOS" REQUIRED)

        IF(Vulkan_LIBRARY)
            set(Vulkan_FOUND ON)
            MESSAGE(STATUS "Using bundled Vulkan library version")
        ENDIF()
    ENDIF()
ENDIF(WIN32)

if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan not found!")
endif()

message(STATUS "Found vulkan library at ${Vulkan_LIBRARY}")

if(NOT Vulkan_INCLUDE_DIRS)
    set(Vulkan_INCLUDE_DIRS "${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/Vulkan/include")
    set(Vulkan_INCLUDE_DIR "${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/Vulkan/include")
    message(STATUS "Using bundled Vulkan Include Headers!")
endif()

# Find GLSL Language Validator
if(NOT Vulkan_glslc_FOUND)
    message(WARNING "GLSL Compiler not found!")
else()
    message(STATUS "Found GLSL_COMPILER at ${Vulkan_GLSLC_EXECUTABLE}")
endif()

# Now that every package required is found, setup the build environment
list(APPEND FBY_LIBRARY_DEPENDENCIES ${Vulkan_LIBRARY})
list(APPEND FBY_INCLUDE_DIRS ${Vulkan_INCLUDE_DIRS})

# Setting All the required compile definitions
set(FBY_COMPILE_DEFINITIONS FBY_PROJECT_DIR="${FBY_SOURCE_DIR}/" GLFW_INCLUDE_VULKAN GLM_FORCE_DEPTH_ZERO_TO_ONE)

# Setting the paths we require irrespective of the Graphics API
list(APPEND FBY_LIBRARY_DEPENDENCIES glfw yaml-cpp fmt spirv-reflect-static)
list(APPEND FBY_INCLUDE_DIRS
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/GLFW/include
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/glm
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/imgui
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/YamlCpp/include
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/fmtlib/include
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/Assimp/include
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/Assimp/build/include
	${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/Mono/include
)

# Nvidia PhysX
include(Flameberry/Build/CMake/envPhysX.cmake)

add_library(PhysX_LIBRARY STATIC IMPORTED)
set_target_properties(PhysX_LIBRARY PROPERTIES IMPORTED_LOCATION_DEBUG ${PHYSX_CHECKED_LIB_DIRECTORY}/libPhysX_static_64.a)
set_target_properties(PhysX_LIBRARY PROPERTIES IMPORTED_LOCATION_RELEASE ${PHYSX_RELEASE_LIB_DIRECTORY}/libPhysX_static_64.a)

add_library(PhysXFoundation_LIBRARY STATIC IMPORTED)
set_target_properties(PhysXFoundation_LIBRARY PROPERTIES IMPORTED_LOCATION_DEBUG ${PHYSX_CHECKED_LIB_DIRECTORY}/libPhysXFoundation_static_64.a)
set_target_properties(PhysXFoundation_LIBRARY PROPERTIES IMPORTED_LOCATION_RELEASE ${PHYSX_RELEASE_LIB_DIRECTORY}/libPhysXFoundation_static_64.a)

add_library(PhysXCommon_LIBRARY STATIC IMPORTED)
set_target_properties(PhysXCommon_LIBRARY PROPERTIES IMPORTED_LOCATION_DEBUG ${PHYSX_CHECKED_LIB_DIRECTORY}/libPhysXCommon_static_64.a)
set_target_properties(PhysXCommon_LIBRARY PROPERTIES IMPORTED_LOCATION_RELEASE ${PHYSX_RELEASE_LIB_DIRECTORY}/libPhysXCommon_static_64.a)

add_library(PhysXCooking_LIBRARY STATIC IMPORTED)
set_target_properties(PhysXCooking_LIBRARY PROPERTIES IMPORTED_LOCATION_DEBUG ${PHYSX_CHECKED_LIB_DIRECTORY}/libPhysXCooking_static_64.a)
set_target_properties(PhysXCooking_LIBRARY PROPERTIES IMPORTED_LOCATION_RELEASE ${PHYSX_RELEASE_LIB_DIRECTORY}/libPhysXCooking_static_64.a)

add_library(PhysXExtensions_LIBRARY STATIC IMPORTED)
set_target_properties(PhysXExtensions_LIBRARY PROPERTIES IMPORTED_LOCATION_DEBUG ${PHYSX_CHECKED_LIB_DIRECTORY}/libPhysXExtensions_static_64.a)
set_target_properties(PhysXExtensions_LIBRARY PROPERTIES IMPORTED_LOCATION_RELEASE ${PHYSX_RELEASE_LIB_DIRECTORY}/libPhysXExtensions_static_64.a)

# TODO: Should not be needed
add_library(PhysXPvdSDK_LIBRARY STATIC IMPORTED)
set_target_properties(PhysXPvdSDK_LIBRARY PROPERTIES IMPORTED_LOCATION_DEBUG ${PHYSX_CHECKED_LIB_DIRECTORY}/libPhysXPvdSDK_static_64.a)
set_target_properties(PhysXPvdSDK_LIBRARY PROPERTIES IMPORTED_LOCATION_RELEASE ${PHYSX_RELEASE_LIB_DIRECTORY}/libPhysXPvdSDK_static_64.a)

list(APPEND FBY_LIBRARY_DEPENDENCIES PhysX_LIBRARY PhysXFoundation_LIBRARY PhysXCommon_LIBRARY PhysXCooking_LIBRARY PhysXExtensions_LIBRARY PhysXPvdSDK_LIBRARY)
list(APPEND FBY_INCLUDE_DIRS ${PHYSX_INCLUDE_DIR})
list(APPEND FBY_COMPILE_DEFINITIONS ${PHYSX_COMPILE_DEFINITIONS})

# Assimp
find_library(Assimp_LIBRARY NAMES assimp HINTS "${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/Assimp/build/bin" REQUIRED)
list(APPEND FBY_LIBRARY_DEPENDENCIES ${Assimp_LIBRARY})

# ImGui paths and source
file(GLOB IMGUI_SRC ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/imgui/*.cpp ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/imgui/*.h)

# Mono library
find_library(Monosgen_LIBRARY NAMES monosgen-2.0 HINTS "${FBY_SOURCE_DIR}/Flameberry/Binaries/ThirdParty/Mono/MacOS" REQUIRED)
list(APPEND FBY_LIBRARY_DEPENDENCIES ${Monosgen_LIBRARY})

set(FBY_DEPENDENCY_SOURCE

    # ImGui
    ${IMGUI_SRC}
    ${CMAKE_SOURCE_DIR}/Flameberry/Source/ThirdParty/imgui/backends/imgui_impl_vulkan.cpp
    ${CMAKE_SOURCE_DIR}/Flameberry/Source/ThirdParty/imgui/backends/imgui_impl_vulkan.h
    ${CMAKE_SOURCE_DIR}/Flameberry/Source/ThirdParty/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/Flameberry/Source/ThirdParty/imgui/backends/imgui_impl_glfw.h

    # ImGuizmo
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/ImGuizmo/GraphEditor.cpp
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/ImGuizmo/GraphEditor.h
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/ImGuizmo/ImCurveEdit.cpp
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/ImGuizmo/ImCurveEdit.h
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/ImGuizmo/ImGradient.cpp
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/ImGuizmo/ImGradient.h
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/ImGuizmo/ImGuizmo.cpp
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/ImGuizmo/ImGuizmo.h
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/ImGuizmo/ImSequencer.cpp
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/ImGuizmo/ImSequencer.h
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/ImGuizmo/ImZoomSlider.h

    # Murmur Hash
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/MurmurHash/MurmurHash3.h
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/MurmurHash/MurmurHash3.cpp
)
