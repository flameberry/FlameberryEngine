# Get all shader files in shaders directory
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Vulkan/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/Vulkan/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/Vulkan/*.comp"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    message(STATUS "Adding ${FILE_NAME}.spv shader to compile target...")
    set(SPIRV "${CMAKE_CURRENT_SOURCE_DIR}/Vulkan/Compiled/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL}
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    CompileShadersTarget
    DEPENDS ${SPIRV_BINARY_FILES}
)