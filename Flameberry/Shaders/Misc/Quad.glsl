#shader vertex
#version 410 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;
layout (location = 2) in vec2 a_Texture_UV;
layout (location = 3) in float a_TextureIndex;
layout (location = 4) in int a_EntityID;

out vec4 v_Color;
out float v_TextureIndex;
out vec2 v_Texture_UV;
flat out int  v_EntityID;

layout (std140) uniform Camera
{
    mat4 ViewProjectionMatrix;
} u_Camera;

void main()
{
    v_Color = a_Color;
    v_TextureIndex = a_TextureIndex;
    v_Texture_UV = a_Texture_UV;
    v_EntityID = a_EntityID;

    gl_Position = u_Camera.ViewProjectionMatrix * vec4(a_Position, 1.0);
}

#shader fragment
#version 410 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out int o_EntityID;

in vec4 v_Color;
in float v_TextureIndex;
in vec2 v_Texture_UV;
flat in int v_EntityID;

uniform sampler2D u_TextureSamplers[16];

void main()
{
    o_EntityID = v_EntityID;
    if (v_TextureIndex == -1)
        FragColor = v_Color;
    else 
        FragColor = texture(u_TextureSamplers[int(v_TextureIndex)], v_Texture_UV);
}
