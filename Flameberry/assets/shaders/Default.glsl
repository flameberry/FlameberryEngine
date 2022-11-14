#shader vertex
#version 410 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;
layout (location = 2) in vec3 a_Normal;
layout (location = 3) in vec2 a_Texture_UV;
layout (location = 4) in float a_TextureIndex;
layout (location = 5) in int a_EntityID;

out vec3 v_Position;
out vec4 v_Color;
out vec3 v_Normal;
out vec2 v_Texture_UV;
out float v_TextureIndex;
flat out int v_EntityID;

uniform mat4 u_ModelMatrix;

// temp
out mat4 v_ModelMatrix;

layout (std140) uniform Camera
{
    mat4 ViewProjectionMatrix;
} u_Camera;

void main()
{
    // temp
    v_ModelMatrix = u_ModelMatrix;

    v_Position = a_Position;
    v_Color = a_Color;
    v_Normal = a_Normal;
    v_Texture_UV = a_Texture_UV;
    v_TextureIndex = a_TextureIndex;
    v_EntityID = a_EntityID;

    // mat3 normalMatrix = inverse(mat3(u_ModelMatrix));
    // v_Normal = normalize(v_Normal * normalMatrix);

    gl_Position = u_Camera.ViewProjectionMatrix * u_ModelMatrix * vec4(a_Position, 1.0);
}

#shader fragment
#version 410 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out int o_EntityID;

in vec3 v_Position;
in vec4 v_Color;
in vec3 v_Normal;
in vec2 v_Texture_UV;
in float v_TextureIndex;
flat in int v_EntityID;

in mat4 v_ModelMatrix;

struct PointLight
{
    vec3 Position;
    vec4 Color;
    float Intensity;
};

uniform sampler2D u_TextureSamplers[16];

uniform int u_LightCount;
uniform PointLight u_PointLights[10];

void main()
{
    o_EntityID = v_EntityID;

    vec3 color = vec3(0.0);
    for (int i = 0; i < u_LightCount; i++)
    {
        PointLight pointLight = u_PointLights[i];
        float ambient = 0.0;
        vec3 lightColor = vec3(pointLight.Color.xyz);
        vec3 lightDir = v_Position - pointLight.Position;
        float distance = length(lightDir);
        lightDir = normalize(lightDir);

        float constant = 1.0, linear = 1.0, exp = 1.0;
        float attenuation = constant + linear * distance + exp * distance * distance;

        float lightIntensity = clamp(dot(v_Normal, -lightDir), 0.0, 1.0);
        vec3 lightMultiplier = lightColor * lightIntensity;

        color += (lightMultiplier * pointLight.Intensity + ambient) / attenuation;
    }

    // Do the light calculations
    // float ambient = 0.5;
    // vec3 lightColor = vec3(u_PointLight.Color.xyz);
    // vec3 lightDir = v_Position - u_PointLight.Position;
    // float distance = length(lightDir);
    // lightDir = normalize(lightDir);

    // float constant = 1.0, linear = 1.0, exp = 1.0;
    // float attenuation = constant + linear * distance + exp * distance * distance;

    // float lightIntensity = clamp(dot(v_Normal, -lightDir), 0.0, 1.0);
    // vec3 lightMultiplier = lightColor * lightIntensity;

    // if (v_TextureIndex == -1)
    //     FragColor = v_Color * vec4(lightMultiplier + ambient, 1.0);
    // else 
    //     FragColor = texture(u_TextureSamplers[int(v_TextureIndex)], v_Texture_UV) * vec4((lightMultiplier * u_PointLight.Intensity + ambient) / attenuation, 1.0);

    if (v_TextureIndex == -1)
        FragColor = v_Color * vec4(color, 1.0);
    else 
        FragColor = texture(u_TextureSamplers[int(v_TextureIndex)], v_Texture_UV) * vec4(color, 1.0);

    // FragColor = v_Color * vec4(color, 1.0);
    // FragColor = vec4(color, 1.0);
    FragColor = clamp(FragColor, 0.0, 1.0);
}
