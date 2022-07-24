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