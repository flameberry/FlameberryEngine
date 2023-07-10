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
set(FL_COMPILE_DEFINITIONS FL_PROJECT_DIR="${FL_SOURCE_DIR}" GLFW_INCLUDE_VULKAN GLM_FORCE_DEPTH_ZERO_TO_ONE)

# Setting the paths we require irrespective of the Graphics API
list(APPEND FL_GRAPHICS_LIBS glfw yaml-cpp)
list(APPEND FL_GRAPHICS_INCLUDE_DIRS 
    ${FL_SOURCE_DIR}/Flameberry/vendor
    ${FL_SOURCE_DIR}/Flameberry/vendor/GLFW/include
    ${FL_SOURCE_DIR}/Flameberry/vendor/glm
    ${FL_SOURCE_DIR}/Flameberry/vendor/imgui
    ${FL_SOURCE_DIR}/Flameberry/vendor/yaml-cpp/include
)

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