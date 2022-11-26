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

uniform vec3 u_CameraPosition;

void main()
{
    o_EntityID = v_EntityID;

    vec3 color = vec3(0.0);
    for (int i = 0; i < u_LightCount; i++)
    {
        PointLight pointLight = u_PointLights[i];

        float ambientIntensity = 0.2;
        vec3 ambientColor = ambientIntensity * pointLight.Color.xyz;

        vec3 lightDir = v_Position - pointLight.Position;
        float distance = length(lightDir);
        lightDir = normalize(lightDir);

        vec3 diffuseColor = vec3(0.0);
        vec3 specularColor = vec3(0.0);
        float diffuseFactor = dot(v_Normal, -lightDir);

        if (diffuseFactor >= 0.0)
            diffuseColor = pointLight.Color.xyz * diffuseFactor * pointLight.Intensity;

        vec3 pixelToCamera = normalize(u_CameraPosition - v_Position);
        vec3 reflectedLight = normalize(reflect(lightDir, v_Normal));
        float specularFactor = dot(pixelToCamera, reflectedLight);

        if (specularFactor >= 0.0)
        {
            float specularExponent = 16.0;
            // float specularExponent = texture(u_TextureSamplers[1], v_Texture_UV).r * 255.0;
            // specularFactor = pow(specularFactor, specularExponent) * texture(u_TextureSamplers[1], v_Texture_UV).r;
            specularFactor = pow(specularFactor, specularExponent);

            float specularIntensity = 5.0;
            specularColor = pointLight.Color.xyz * specularFactor * specularIntensity;
        }

        float constant = 1.0, linear = 1.0, exp = 0.2;
        float attenuation = constant + linear * distance + exp * distance * distance;

        color += (ambientColor + diffuseColor + specularColor) / attenuation;
        // color += (ambientColor + diffuseColor) / attenuation;
    }

    if (v_TextureIndex == -1)
        FragColor = v_Color * vec4(color, 1.0);
    else 
        FragColor = texture(u_TextureSamplers[int(v_TextureIndex)], v_Texture_UV) * vec4(color, 1.0);

    // FragColor = v_Color * vec4(color, 1.0);
    // FragColor = texture(u_TextureSamplers[int(v_TextureIndex)], v_Texture_UV);

    FragColor = clamp(FragColor, 0.0, 1.0);
}
