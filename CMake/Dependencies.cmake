# Set Global Variables
set(FL_GRAPHICS_API "Vulkan")
set(FL_GRAPHICS_LIBS)
set(FL_GRAPHICS_INCLUDE_DIRS)

# Vulkan Helper Libs
find_package(Vulkan)

IF(UNIX AND NOT APPLE)
    set(LINUX TRUE)
ENDIF()

IF(WIN32)
	IF (NOT Vulkan_FOUND)
        # If 64 bit compiler then use 64 bit version of vulkan library
        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(VULKAN_LIB_PATH "${FL_SOURCE_DIR}/Flameberry/vendor/vulkan/lib/x64")
        else()
            set(VULKAN_LIB_PATH "${FL_SOURCE_DIR}/Flameberry/vendor/vulkan/lib/x86")
        endif()

		find_library(Vulkan_LIBRARY NAMES vulkan-1 PATHS ${VULKAN_LIB_PATH} REQUIRED)
		IF (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
			MESSAGE(STATUS "Using bundled Vulkan library version")
		ENDIF()
	ENDIF()
ELSEIF(LINUX)
	IF (NOT Vulkan_FOUND)
		find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${FL_SOURCE_DIR}/Flameberry/vendor/vulkan/lib/linux" REQUIRED)
		IF (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
			MESSAGE(STATUS "Using bundled Vulkan library version")
		ENDIF()
	ENDIF()
ELSEIF(APPLE)
    IF (NOT Vulkan_FOUND)
		find_library(Vulkan_LIBRARY NAMES vulkan HINTS "${FL_SOURCE_DIR}/Flameberry/vendor/vulkan/lib/macOS" REQUIRED)
		IF (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
			MESSAGE(STATUS "Using bundled Vulkan library version")
		ENDIF()
	ENDIF()
ENDIF(WIN32)

if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan not found!")
endif()

message(STATUS "Found vulkan library at ${Vulkan_LIBRARY}")

if (NOT Vulkan_INCLUDE_DIRS)
    set(Vulkan_INCLUDE_DIRS "${FL_SOURCE_DIR}/Flameberry/vendor/vulkan/include")
    set(Vulkan_INCLUDE_DIR "${FL_SOURCE_DIR}/Flameberry/vendor/vulkan/include")
    message(STATUS "Using bundled Vulkan Include Headers!")
endif()

# Find GLSL Language Validator
if (NOT Vulkan_glslangValidator_FOUND)
    message(WARNING "GLSL Language Validator not found!")
else()
    message(STATUS "Found GLSL_VALIDATOR at ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}")
endif()

# Now that every package required is found, setup the build environment
list(APPEND FL_GRAPHICS_LIBS ${Vulkan_LIBRARY})
list(APPEND FL_GRAPHICS_INCLUDE_DIRS ${Vulkan_INCLUDE_DIRS})

# Setting All the required compile definitions
set(FL_COMPILE_DEFINITIONS FL_PROJECT_DIR="${FL_SOURCE_DIR}/" GLFW_INCLUDE_VULKAN GLM_FORCE_DEPTH_ZERO_TO_ONE)

# Setting the paths we require irrespective of the Graphics API
list(APPEND FL_GRAPHICS_LIBS glfw yaml-cpp)
list(APPEND FL_GRAPHICS_INCLUDE_DIRS 
    ${FL_SOURCE_DIR}/Flameberry/vendor
    ${FL_SOURCE_DIR}/Flameberry/vendor/GLFW/include
    ${FL_SOURCE_DIR}/Flameberry/vendor/glm
    ${FL_SOURCE_DIR}/Flameberry/vendor/imgui
    ${FL_SOURCE_DIR}/Flameberry/vendor/yaml-cpp/include
)

# Nvidia PhysX
include(CMake/envPhysX.cmake)

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

list(APPEND FL_GRAPHICS_LIBS PhysX_LIBRARY PhysXFoundation_LIBRARY PhysXCommon_LIBRARY PhysXCooking_LIBRARY PhysXExtensions_LIBRARY PhysXPvdSDK_LIBRARY)
list(APPEND FL_GRAPHICS_INCLUDE_DIRS ${PHYSX_INCLUDE_DIR})
list(APPEND FL_COMPILE_DEFINITIONS ${PHYSX_COMPILE_DEFINITIONS})

# ImGui paths and source
file(GLOB IMGUI_SRC ${FL_SOURCE_DIR}/Flameberry/vendor/imgui/*.cpp ${FL_SOURCE_DIR}/Flameberry/vendor/imgui/*.h)

list(APPEND IMGUI_SRC
    ${CMAKE_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_vulkan.cpp
    ${CMAKE_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_vulkan.h
    ${CMAKE_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_glfw.h
)

# ImGuizmo paths and source
list(APPEND IMGUIZMO_SRC
    ${FL_SOURCE_DIR}/Flameberry/vendor/ImGuizmo/GraphEditor.cpp
    ${FL_SOURCE_DIR}/Flameberry/vendor/ImGuizmo/GraphEditor.h
    ${FL_SOURCE_DIR}/Flameberry/vendor/ImGuizmo/ImCurveEdit.cpp
    ${FL_SOURCE_DIR}/Flameberry/vendor/ImGuizmo/ImCurveEdit.h
    ${FL_SOURCE_DIR}/Flameberry/vendor/ImGuizmo/ImGradient.cpp
    ${FL_SOURCE_DIR}/Flameberry/vendor/ImGuizmo/ImGradient.h
    ${FL_SOURCE_DIR}/Flameberry/vendor/ImGuizmo/ImGuizmo.cpp
    ${FL_SOURCE_DIR}/Flameberry/vendor/ImGuizmo/ImGuizmo.h
    ${FL_SOURCE_DIR}/Flameberry/vendor/ImGuizmo/ImSequencer.cpp
    ${FL_SOURCE_DIR}/Flameberry/vendor/ImGuizmo/ImSequencer.h
    ${FL_SOURCE_DIR}/Flameberry/vendor/ImGuizmo/ImZoomSlider.h
)