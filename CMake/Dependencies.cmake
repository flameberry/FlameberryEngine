# Set Global Variables

# Configure the API used  to build the app [Uses OpenGL if not specified in CMake Generation process
# set(FL_GRAPHICS_API "OpenGL")
set(FL_GRAPHICS_API "Vulkan")

# Vulkan path remains empty if Opengl is selected as the Graphics API
set(VULKAN_PATH "")

# ImGui paths and source, this will be modified depending on the Graphics API
file(GLOB IMGUI_SRC ${FL_SOURCE_DIR}/Flameberry/vendor/imgui/*.cpp ${FL_SOURCE_DIR}/Flameberry/vendor/imgui/*.h)

# Setting the paths we require irrespective of the Graphics API
set(FL_GRAPHICS_LIBS glfw)
set(FL_GRAPHICS_INCLUDE_DIRS 
    ${FL_SOURCE_DIR}/Flameberry/vendor
    ${FL_SOURCE_DIR}/Flameberry/vendor/GLFW/include
    ${FL_SOURCE_DIR}/Flameberry/vendor/glm
    ${FL_SOURCE_DIR}/Flameberry/vendor/imgui
)

set(FL_COMPILE_DEFINITIONS FL_PROJECT_DIR="${FL_SOURCE_DIR}" GLFW_INCLUDE_NONE)

# Graphics API dependent changes begin here
if (FL_GRAPHICS_API STREQUAL "Vulkan")
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

    if (APPLE)
        string(REGEX REPLACE "/include" "" FL_VULKAN_PATH ${FL_VULKAN_PATH})

        set(CMAKE_XCODE_SCHEME_ENVIRONMENT VK_ICD_FILENAMES="${FL_VULKAN_PATH}/share/vulkan/icd.d/MoltenVK_icd.json")
        set(CMAKE_XCODE_SCHEME_ENVIRONMENT VK_LAYER_PATH="${FL_VULKAN_PATH}/share/vulkan/explicit_layer.d")
    elseif(WIN32)
        string(REGEX REPLACE "/Include" "" FL_VULKAN_PATH ${FL_VULKAN_PATH})
    endif()

    list(APPEND FL_COMPILE_DEFINITIONS FL_USE_VULKAN_API GLFW_INCLUDE_VULKAN)
    
    list(APPEND IMGUI_SRC
    ${CMAKE_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_vulkan.cpp
    ${CMAKE_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_vulkan.h
    ${CMAKE_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_glfw.h
    )
    elseif(FL_GRAPHICS_API STREQUAL "OpenGL")
    # OpenGL Helper Libs
    message("-- Using OpenGL Graphics API")
    list(APPEND FL_GRAPHICS_LIBS Glad)
    
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
    
    list(APPEND FL_GRAPHICS_INCLUDE_DIRS ${FL_SOURCE_DIR}/Flameberry/vendor/Glad/include)
    
    list(APPEND FL_COMPILE_DEFINITIONS FL_USE_OPENGL_API GLFW_INCLUDE_NONE IMGUI_IMPL_OPENGL_LOADER_GLAD)

    list(APPEND IMGUI_SRC
    ${FL_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_glfw.cpp
    ${FL_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_glfw.h
    ${FL_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_opengl3.cpp
    ${FL_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_opengl3.h
    ${FL_SOURCE_DIR}/Flameberry/vendor/imgui/backends/imgui_impl_opengl3_loader.h
    )
    endif()
    
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