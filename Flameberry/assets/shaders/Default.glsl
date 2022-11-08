#shader vertex
#version 410 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;
layout (location = 2) in vec3 a_Normal;
layout (location = 3) in vec2 a_Texture_UV;
layout (location = 4) in float a_TextureIndex;
layout (location = 5) in int a_EntityID;

out vec4 v_Color;
out vec3 v_Normal;
out vec2 v_Texture_UV;
out float v_TextureIndex;
flat out int v_EntityID;

uniform mat4 u_ModelMatrix;

layout (std140) uniform Camera
{
    mat4 ViewProjectionMatrix;
} u_Camera;

void main()
{
    v_Color = a_Color;
    v_Normal = a_Normal;
    v_Texture_UV = a_Texture_UV;
    v_TextureIndex = a_TextureIndex;
    v_EntityID = a_EntityID;

    mat3 normalMatrix = inverse(mat3(u_ModelMatrix));
    v_Normal = normalize(v_Normal * normalMatrix);

    gl_Position = u_Camera.ViewProjectionMatrix * u_ModelMatrix * vec4(a_Position, 1.0);
}

#shader fragment
#version 410 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out int o_EntityID;

in vec4 v_Color;
in vec3 v_Normal;
in vec2 v_Texture_UV;
in float v_TextureIndex;
flat in int v_EntityID;

uniform sampler2D u_TextureSamplers[16];

void main()
{
    o_EntityID = v_EntityID;

    float ambient = 0.2;

    // Do the light calculations
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    vec3 lightDir = vec3(-1, -1, -1);
    float lightIntensity = max(dot(v_Normal, -lightDir), 0.0);

    vec4 lightMultiplier = vec4(lightColor * lightIntensity, 1.0);

    if (v_TextureIndex == -1)
        FragColor = v_Color * (lightMultiplier + ambient);
    else 
        FragColor = texture(u_TextureSamplers[int(v_TextureIndex)], v_Texture_UV) * (lightMultiplier + ambient);

    // FragColor = vec4(v_Normal, 1.0);
}
