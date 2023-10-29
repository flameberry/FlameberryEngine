#version 450

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Color;

layout (location = 0) out vec3 v_Color;
layout (location = 1) out vec2 v_TextureCoords;

layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 u_ViewMatrix, u_ProjectionMatrix, u_ViewProjectionMatrix;
};

const vec2 g_TextureCoords[] = {
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0)
};

void main()
{
    gl_Position = u_ViewProjectionMatrix * vec4(a_Position, 1.0);
    v_Color = a_Color;
    v_TextureCoords = g_TextureCoords[gl_VertexIndex % 4];
}