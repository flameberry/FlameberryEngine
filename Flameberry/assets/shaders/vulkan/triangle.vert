#version 450

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;
layout (location = 2) in vec2 a_TextureCoords;

layout (location = 0) out vec4 v_VertexColor;
layout (location = 1) out vec2 v_TextureCoords;

layout (binding = 0) uniform UniformBufferObject {
    mat4 ModelMatrix;
    mat4 ViewMatrix;
    mat4 ProjectionMatrix;
} u_MVP_Data;

void main()
{
    gl_Position = u_MVP_Data.ProjectionMatrix * u_MVP_Data.ViewMatrix * u_MVP_Data.ModelMatrix * vec4(a_Position, 1.0);
    v_VertexColor = a_Color;
    v_TextureCoords = a_TextureCoords;
}