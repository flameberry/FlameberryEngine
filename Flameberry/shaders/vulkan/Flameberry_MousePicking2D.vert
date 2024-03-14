#version 450

layout (location = 0) in vec3 a_Position;
layout (location = 2) in int a_EntityIndex;

layout (location = 0) out flat int v_EntityIndex;

layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 u_ViewMatrix, u_ProjectionMatrix, u_ViewProjectionMatrix;
};

void main()
{
    v_EntityIndex = a_EntityIndex;
    gl_Position = u_ViewProjectionMatrix * vec4(a_Position, 1.0);
}