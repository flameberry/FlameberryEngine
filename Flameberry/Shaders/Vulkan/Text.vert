#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
layout(location = 2) in vec2 a_TextureCoords;

layout(location = 0) out vec3 v_Color;
layout(location = 1) out vec2 v_TextureCoords;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 u_ViewMatrix, u_ProjectionMatrix, u_ViewProjectionMatrix;
};

void main()
{
    gl_Position = u_ViewProjectionMatrix * vec4(a_Position, 1.0);
    v_Color = a_Color;
    v_TextureCoords = a_TextureCoords;
}
