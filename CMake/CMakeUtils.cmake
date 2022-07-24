# Function to set output directories for the specified target
function(set_custom_build_properties TARGET_NAME)
    set(BUILD_CONFIG "")
    string(APPEND BUILD_CONFIG $<$<CONFIG:Debug>:Debug> $<$<CONFIG:Release>:Release>)
    set_target_properties(${TARGET_NAME}
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${FL_SOURCE_DIR}/bin/${BUILD_CONFIG}/${TARGET_NAME}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}/
        LIBRARY_OUTPUT_DIRECTORY ${FL_SOURCE_DIR}/bin/${BUILD_CONFIG}/${TARGET_NAME}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}/
        RUNTIME_OUTPUT_DIRECTORY ${FL_SOURCE_DIR}/bin/${BUILD_CONFIG}/${TARGET_NAME}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}/
        OUTPUT_NAME "${TARGET_NAME}_${BUILD_CONFIG}"
    )
endfunction()

# Function to define c++ macro "FL_DEBUG" in debug config and "FL_RELEASE" in release config, for the given target
function(set_build_config_macro_for_target TARGET_NAME)
    set(BUILD_CONFIG_MACRO "")
    set(BUILD_CONFIG_DEBUG_MACRO $<$<CONFIG:Debug>:FL_DEBUG>)
    set(BUILD_CONFIG_RELEASE_MACRO $<$<CONFIG:Release>:FL_RELEASE>)
    string(APPEND BUILD_CONFIG_MACRO ${BUILD_CONFIG_DEBUG_MACRO} ${BUILD_CONFIG_RELEASE_MACRO})
    target_compile_definitions(${TARGET_NAME} PRIVATE ${BUILD_CONFIG_MACRO})
endfunction()

