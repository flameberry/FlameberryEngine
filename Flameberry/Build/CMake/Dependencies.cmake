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
list(APPEND FBY_LIBRARY_DEPENDENCIES glfw yaml-cpp fmt spirv-reflect-static assimp msdf-atlas-gen Jolt)
list(APPEND FBY_INCLUDE_DIRS
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/GLFW/include
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/glm
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/imgui
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/YamlCpp/include
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/fmtlib/include
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/Assimp/include
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/Assimp/build/include
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/MsdfAtlasGen/MsdfAtlasGen/msdf-atlas-gen
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/MsdfAtlasGen/MsdfAtlasGen/msdfgen
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/JoltPhysics
)

# ImGui paths and source
file(GLOB IMGUI_SRC
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/imgui/*.cpp
    ${FBY_SOURCE_DIR}/Flameberry/Source/ThirdParty/imgui/*.h
)

set(FBY_DEPENDENCY_SOURCE

    # ImGui
    ${IMGUI_SRC}
    ${CMAKE_SOURCE_DIR}/Flameberry/Source/ThirdParty/imgui/backends/imgui_impl_vulkan.cpp
    ${CMAKE_SOURCE_DIR}/Flameberry/Source/ThirdParty/imgui/backends/imgui_impl_vulkan.h
    ${CMAKE_SOURCE_DIR}/Flameberry/Source/ThirdParty/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/Flameberry/Source/ThirdParty/imgui/backends/imgui_impl_glfw.h

    ${CMAKE_SOURCE_DIR}/Flameberry/Source/ThirdParty/imgui/misc/cpp/imgui_stdlib.cpp
    ${CMAKE_SOURCE_DIR}/Flameberry/Source/ThirdParty/imgui/misc/cpp/imgui_stdlib.h

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
