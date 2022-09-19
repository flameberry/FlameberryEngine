#version 450

layout (location = 0) in vec4 v_VertexColor;
layout (location = 1) in vec2 v_TextureCoords;

layout (location = 0) out vec4 FragColor;

void main()
{
    FragColor = vec4(v_TextureCoords, 0.0, 1.0);
}
