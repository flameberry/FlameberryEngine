#version 450

#extension GL_EXT_multiview : require

#define CASCADE_COUNT 4

layout (location = 0) in vec3 a_Position;

layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 u_ViewProjectionMatrix[CASCADE_COUNT];
};

layout (push_constant) uniform ModelMatrix {
    mat4 u_ModelMatrix;
};

void main()
{
    gl_Position = u_ViewProjectionMatrix[gl_ViewIndex] * u_ModelMatrix * vec4(a_Position, 1.0);
}