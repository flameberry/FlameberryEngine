# Add Source Targets
add_subdirectory(Developer)
add_subdirectory(Editor)

# Add Dependencies
add_subdirectory(ThirdParty/GLFW)
add_subdirectory(ThirdParty/YamlCpp)
add_subdirectory(ThirdParty/fmtlib)

option(SPIRV_REFLECT_EXECUTABLE "Build spirv-reflect executable" OFF)
option(SPIRV_REFLECT_STATIC_LIB "Build a SPIRV-Reflect static library" ON)

add_subdirectory(ThirdParty/SPIRV-Reflect)