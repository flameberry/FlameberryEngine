#version 450

layout (location = 0) in vec3 a_Position;

layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 u_ViewProjectionMatrix;
};

layout (push_constant) uniform MousePickingPushConstant {
    mat4 u_ModelMatrix;
    int u_EntityIndex;
};

void main()
{
    gl_Position = u_ViewProjectionMatrix * u_ModelMatrix * vec4(a_Position, 1.0);
}