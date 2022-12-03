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

    v_Normal = mat3(transpose(inverse(u_ModelMatrix))) * v_Normal;
    v_Position = vec3(u_ModelMatrix * vec4(v_Position, 1.0));
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

struct DirectionalLight
{
    vec3 Direction;
    vec4 Color;
    float Intensity;
};

struct PointLight
{
    vec3 Position;
    vec4 Color;
    float Intensity;
};

struct Material
{
    vec3 Albedo;
    float Roughness;
    bool IsMetal;
};

// layout (std140) uniform Lighting
// {
//     DirectionalLight DirLight;
//     PointLight PointLights[10];
//     int LightCount;
// } u_Lighting;

uniform vec3 u_CameraPosition;

uniform sampler2D u_TextureSamplers[16];

uniform int u_LightCount;
uniform PointLight u_PointLights[10];
uniform DirectionalLight u_DirectionalLight;
uniform Material u_Material;

#define PI 3.1415926535897932384626433832795

// PBR Lighting
vec3 SchlickFresnel(float v_dot_h)
{
    vec3 F0 = vec3(0.04);

    if (u_Material.IsMetal) {
        F0 = u_Material.Albedo;
    }

    vec3 ret = F0 + (1 - F0) * pow(clamp(1.0 - v_dot_h, 0.0, 1.0), 5);
    return ret;
}


float GeomSmith(float dp)
{
    float k = (u_Material.Roughness + 1.0) * (u_Material.Roughness + 1.0) / 8.0;
    float denom = dp * (1 - k) + k;
    return dp / denom;
}


float GGXDistribution(float n_dot_h)
{
    float alpha2 = pow(u_Material.Roughness, 4);
    float d = n_dot_h * n_dot_h * (alpha2 - 1) + 1;
    float ggxdistrib = alpha2 / (PI * d * d);
    return ggxdistrib;
}

vec3 CalculatePBRDirectionalLight(DirectionalLight light, vec3 normal)
{
    vec3 lightIntensity = light.Color.xyz * light.Intensity;
    
    vec3 l = normalize(-light.Direction.xyz);
    
    vec3 n = normal;
    vec3 v = normalize(u_CameraPosition - v_Position);
    vec3 h = normalize(v + l);
    
    float n_dot_h = max(dot(n, h), 0.0);
    float v_dot_h = max(dot(v, h), 0.0);
    float n_dot_l = max(dot(n, l), 0.0);
    float n_dot_v = max(dot(n, v), 0.0);
    
    vec3 fresnelFactor = SchlickFresnel(v_dot_h);
    vec3 kS = fresnelFactor;
    vec3 kD = 1.0 - kS;
    
    vec3 specularBRDFNumerator = GGXDistribution(n_dot_h) * fresnelFactor * GeomSmith(n_dot_l) * GeomSmith(n_dot_v);
    float specularBRDFDenominator = 4.0 * n_dot_v * n_dot_l + 0.0001;
    
    vec3 specularBRDF = specularBRDFNumerator / specularBRDFDenominator;
    
    vec3 fLambert = vec3(0.0);
    
    if (!u_Material.IsMetal)
    {
        fLambert = u_Material.Albedo;
    }
    
    vec3 diffuseBRDF = kD * fLambert / PI;
    
    vec3 finalColor = (diffuseBRDF + specularBRDF) * lightIntensity * n_dot_l;
    return finalColor;
}

vec3 CalculatePBRPointLight(PointLight light, vec3 normal)
{
    vec3 lightIntensity = light.Color.xyz * light.Intensity;
    
    vec3 l = vec3(0.0);
    l = light.Position - v_Position;
    float lightToPixelDistance = length(l);
    l = normalize(l);
    lightIntensity /= lightToPixelDistance * lightToPixelDistance;
    
    vec3 n = normal;
    vec3 v = normalize(u_CameraPosition - v_Position);
    vec3 h = normalize(v + l);
    
    float n_dot_h = max(dot(n, h), 0.0);
    float v_dot_h = max(dot(v, h), 0.0);
    float n_dot_l = max(dot(n, l), 0.0);
    float n_dot_v = max(dot(n, v), 0.0);
    
    vec3 fresnelFactor = SchlickFresnel(v_dot_h);
    vec3 kS = fresnelFactor;
    vec3 kD = 1.0 - kS;
    
    vec3 specularBRDFNumerator = GGXDistribution(n_dot_h) * fresnelFactor * GeomSmith(n_dot_l) * GeomSmith(n_dot_v);
    float specularBRDFDenominator = 4.0 * n_dot_v * n_dot_l + 0.0001;
    
    vec3 specularBRDF = specularBRDFNumerator / specularBRDFDenominator;
    
    vec3 fLambert = vec3(0.0);
    
    if (!u_Material.IsMetal)
    {
        fLambert = u_Material.Albedo;
    }
    
    vec3 diffuseBRDF = kD * fLambert / PI;
    
    vec3 finalColor = (diffuseBRDF + specularBRDF) * lightIntensity * n_dot_l;
    return finalColor;
}

vec4 CalculatePBRLighting()
{
    vec3 normal = normalize(v_Normal);

    vec3 totalLight = CalculatePBRDirectionalLight(u_DirectionalLight, normal);
    for (int i = 0; i < u_LightCount; i++)
        totalLight += CalculatePBRPointLight(u_PointLights[i], normal);
    
    // HDR tone mapping
    totalLight = totalLight / (totalLight + vec3(1.0));
    
    // Gamma correction
    return vec4(pow(totalLight, vec3(1.0 / 2.2)), 1.0);
}

void main()
{
    o_EntityID = v_EntityID;

//    vec3 color = vec3(0.0);
//    for (int i = 0; i < u_LightCount; i++)
//    {
//        PointLight pointLight = u_PointLights[i];
//
//        float ambientIntensity = 0.2;
//        vec3 ambientColor = ambientIntensity * pointLight.Color.xyz;
//
//        vec3 lightDir = v_Position - pointLight.Position;
//        float distance = length(lightDir);
//        lightDir = normalize(lightDir);
//
//        vec3 diffuseColor = vec3(0.0);
//        vec3 specularColor = vec3(0.0);
//        float diffuseFactor = dot(v_Normal, -lightDir);
//
//        if (diffuseFactor >= 0.0)
//            diffuseColor = pointLight.Color.xyz * diffuseFactor * pointLight.Intensity;
//
//        vec3 pixelToCamera = normalize(u_CameraPosition - v_Position);
//        vec3 reflectedLight = normalize(reflect(lightDir, v_Normal));
//        float specularFactor = dot(pixelToCamera, reflectedLight);
//
//        if (specularFactor >= 0.0)
//        {
//            float specularExponent = 16.0;
//            // float specularExponent = texture(u_TextureSamplers[1], v_Texture_UV).r * 255.0;
//            // specularFactor = pow(specularFactor, specularExponent) * texture(u_TextureSamplers[1], v_Texture_UV).r;
//            specularFactor = pow(specularFactor, specularExponent);
//
//            float specularIntensity = 5.0;
//            specularColor = pointLight.Color.xyz * specularFactor * specularIntensity;
//        }
//
//        float constant = 1.0, linear = 1.0, exp = 0.2;
//        float attenuation = constant + linear * distance + exp * distance * distance;
//
//        color += (ambientColor + diffuseColor + specularColor) / attenuation;
//        // color += (ambientColor + diffuseColor) / attenuation;
//    }
//
//    if (v_TextureIndex == -1)
//        FragColor = v_Color * vec4(color, 1.0);
//    else
//        FragColor = texture(u_TextureSamplers[int(v_TextureIndex)], v_Texture_UV) * vec4(color, 1.0);

    // FragColor = v_Color * vec4(color, 1.0);
    // FragColor = texture(u_TextureSamplers[int(v_TextureIndex)], v_Texture_UV);

    // FragColor = clamp(FragColor, 0.0, 1.0);

    FragColor = CalculatePBRLighting();
}
