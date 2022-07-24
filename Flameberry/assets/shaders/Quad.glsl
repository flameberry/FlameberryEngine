#shader vertex
#version 410 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;
layout (location = 2) in vec2 a_Texture_UV;
layout (location = 3) in float a_TextureIndex;
layout (location = 4) in vec2 a_QuadDimensions;
layout (location = 5) in float a_ElementTypeIndex;
layout (location = 6) in float a_IsPanelActive;

out vec4 v_Color;
out float v_TextureIndex;
out vec2 v_Texture_UV;
out vec2 v_QuadDimensions;
out float v_ElementTypeIndex;
out float v_IsPanelActive;

layout (std140) uniform Camera
{
    mat4 ViewProjectionMatrix;
} u_Camera;

void main()
{
    v_Color = a_Color;
    v_TextureIndex = a_TextureIndex;
    v_Texture_UV = a_Texture_UV;
    v_QuadDimensions = a_QuadDimensions;
    v_ElementTypeIndex = a_ElementTypeIndex;
    v_IsPanelActive = a_IsPanelActive;

    gl_Position = u_Camera.ViewProjectionMatrix * vec4(a_Position, 1.0);
}

#shader fragment
#version 410 core

out vec4 FragColor;

in vec4 v_Color;
in float v_TextureIndex;
in vec2 v_Texture_UV;
in vec2 v_QuadDimensions;
in float v_ElementTypeIndex;
in float v_IsPanelActive;

uniform sampler2D u_TextureSamplers[16];

void main()
{
    if (v_TextureIndex == -1)
        FragColor = v_Color;
    else 
        FragColor = texture(u_TextureSamplers[int(v_TextureIndex)], v_Texture_UV);
}
