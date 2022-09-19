#version 450

layout (location = 0) in vec4 v_VertexColor;
layout (location = 1) in vec2 v_TextureCoords;

layout (location = 0) out vec4 FragColor;

layout (binding = 1) uniform sampler2D u_TextureSampler;

void main()
{
    FragColor = texture(u_TextureSampler, v_TextureCoords);
}
