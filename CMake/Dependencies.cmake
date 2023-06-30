# Set Global Variables
set(FL_GRAPHICS_API "Vulkan")

# Vulkan path remains empty if Opengl is selected as the Graphics API
set(VULKAN_PATH "")

# ImGui paths and source, this will be modified depending on the Graphics API
file(GLOB IMGUI_SRC ${FL_SOURCE_DIR}/Flameberry/vendor/imgui/*.cpp ${FL_SOURCE_DIR}/Flameberry/vendor/imgui/*.h)

list(APPEND IMGUI_SRC
    ${CMAKE_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_vulkan.cpp
    ${CMAKE_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_vulkan.h
    ${CMAKE_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_glfw.h
)

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

# Setting the paths we require irrespective of the Graphics API
set(FL_GRAPHICS_LIBS glfw yaml-cpp)
set(FL_GRAPHICS_INCLUDE_DIRS 
    ${FL_SOURCE_DIR}/Flameberry/vendor
    ${FL_SOURCE_DIR}/Flameberry/vendor/GLFW/include
    ${FL_SOURCE_DIR}/Flameberry/vendor/glm
    ${FL_SOURCE_DIR}/Flameberry/vendor/imgui
    ${FL_SOURCE_DIR}/Flameberry/vendor/yaml-cpp/include
)

set(FL_COMPILE_DEFINITIONS FL_PROJECT_DIR="${FL_SOURCE_DIR}" GLFW_INCLUDE_VULKAN GLM_FORCE_DEPTH_ZERO_TO_ONE)

message("-- Using Vulkan Graphics API")

# Vulkan Helper Libs
find_package(Vulkan REQUIRED)

if(NOT Vulkan_FOUND)
message(FATAL_ERROR "-- Vulkan not found!")
endif()

if (${Vulkan_INCLUDE_DIRS} STREQUAL "")
message(FATAL_ERROR "-- Vulkan include directories are empty!")
endif()

list(APPEND FL_GRAPHICS_LIBS Vulkan::Vulkan)
list(APPEND FL_GRAPHICS_INCLUDE_DIRS ${Vulkan_INCLUDE_DIRS})

string(APPEND FL_VULKAN_PATH "${Vulkan_INCLUDE_DIRS}")

# Build Shaders
find_program(GLSL_VALIDATOR glslangValidator HINTS 
    ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} 
    /usr/bin 
    /usr/local/bin 
    ${VULKAN_SDK_PATH}/Bin
    ${VULKAN_SDK_PATH}/Bin32
    $ENV{VULKAN_SDK}/Bin/ 
    $ENV{VULKAN_SDK}/Bin32/
)

message(STATUS "Found GLSL_VALIDATOR at ${GLSL_VALIDATOR}")

if (APPLE)
    string(REGEX REPLACE "/include" "" FL_VULKAN_PATH ${FL_VULKAN_PATH})

    set(CMAKE_XCODE_SCHEME_ENVIRONMENT VK_ICD_FILENAMES="${FL_VULKAN_PATH}/share/vulkan/icd.d/MoltenVK_icd.json")
    set(CMAKE_XCODE_SCHEME_ENVIRONMENT VK_LAYER_PATH="${FL_VULKAN_PATH}/share/vulkan/explicit_layer.d")
elseif(WIN32)
    string(REGEX REPLACE "/Include" "" FL_VULKAN_PATH ${FL_VULKAN_PATH})
endif()