# OpenGL Helper Libs
set(FL_GRAPHICS_LIBS glfw Glad)

if(APPLE)
    # Inbuilt mac frameworks required for GLFW
    list(APPEND FL_GRAPHICS_LIBS 
        "-framework Cocoa"
        "-framework OpenGL"
        "-framework IOKit"
        "-framework CoreFoundation"
    )
elseif(WIN32)
    list(APPEND FL_GRAPHICS_LIBS opengl32.lib)
endif()

# All Include Dirs needed for the project
set(FL_GRAPHICS_INCLUDE_DIRS
    ${FL_SOURCE_DIR}/Flameberry/vendor
    ${FL_SOURCE_DIR}/Flameberry/vendor/GLFW/include
    ${FL_SOURCE_DIR}/Flameberry/vendor/Glad/include
    ${FL_SOURCE_DIR}/Flameberry/vendor/glm
    ${FL_SOURCE_DIR}/Flameberry/vendor/imgui
)

file(GLOB IMGUI_SRC ${FL_SOURCE_DIR}/Flameberry/vendor/imgui/*.cpp ${FL_SOURCE_DIR}/Flameberry/vendor/imgui/*.h)
list(APPEND IMGUI_SRC
    ${FL_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_glfw.cpp
    ${FL_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_glfw.h
    ${FL_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_opengl3.cpp
    ${FL_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_opengl3.h
    ${FL_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_opengl3_loader.h
)

# # Vulkan Required
# find_package(Vulkan REQUIRED)

# if(NOT Vulkan_FOUND)
# message(FATAL_ERROR "-- Vulkan not found!")
# endif()

# if (${Vulkan_INCLUDE_DIRS} STREQUAL "")
# message(FATAL_ERROR "-- Vulkan include directories are empty!")
# endif()

# set(FL_GRAPHICS_LIBS glfw Vulkan::Vulkan)
# set(FL_GRAPHICS_INCLUDE_DIRS 
#     ${CMAKE_SOURCE_DIR}/vendor/GLFW/include
#     ${CMAKE_SOURCE_DIR}/vendor/glm
#     ${CMAKE_SOURCE_DIR}/vendor
#     ${Vulkan_INCLUDE_DIRS}
# )

# set(FL_VULKAN_PATH ${Vulkan_INCLUDE_DIRS})

# if (APPLE)
#     string(REGEX REPLACE "/include" "" FL_VULKAN_PATH ${FL_VULKAN_PATH})

#     set(CMAKE_XCODE_SCHEME_ENVIRONMENT VK_ICD_FILENAMES="${FL_VULKAN_PATH}/share/vulkan/icd.d/MoltenVK_icd.json")
#     set(CMAKE_XCODE_SCHEME_ENVIRONMENT VK_LAYER_PATH="${FL_VULKAN_PATH}/share/vulkan/explicit_layer.d")
# elseif(WIN32)
#     string(REGEX REPLACE "/Include" "" FL_VULKAN_PATH ${FL_VULKAN_PATH})
# endif()