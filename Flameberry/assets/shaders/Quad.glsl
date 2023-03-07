#shader vertex
#version 410 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;
layout (location = 2) in vec2 a_Texture_UV;
layout (location = 3) in float a_TextureIndex;

out vec4 v_Color;
out float v_TextureIndex;
out vec2 v_Texture_UV;

layout (std140) uniform Camera
{
    mat4 u_ViewProjectionMatrix;
    mat4 u_LightViewProjectionMatrix;
};

void main()
{
    v_Color = a_Color;
    v_TextureIndex = a_TextureIndex;
    v_Texture_UV = a_Texture_UV;

    gl_Position = u_ViewProjectionMatrix * vec4(a_Position, 1.0);
}

#shader fragment
#version 410 core

layout (location = 0) out vec4 FragColor;

in vec4 v_Color;
in float v_TextureIndex;
in vec2 v_Texture_UV;

uniform sampler2D u_TextureSamplers[16];

void main()
{
    if (v_TextureIndex == -1)
        FragColor = v_Color;
    else 
        FragColor = texture(u_TextureSamplers[int(v_TextureIndex)], v_Texture_UV);
}