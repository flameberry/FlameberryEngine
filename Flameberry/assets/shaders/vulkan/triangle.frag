#version 450

layout (location = 0) in vec3 v_Position;
layout (location = 1) in vec4 v_VertexColor;
layout (location = 2) in vec3 v_Normal;
layout (location = 3) in vec2 v_TextureCoords;

layout (location = 0) out vec4 FragColor;

#define PI 3.1415926535897932384626433832795

layout (set = 0, binding = 1) uniform sampler2D u_TextureSampler;

struct DirectionalLight {
    vec3  Direction;
    vec3  Color;
    float Intensity;
};

struct PointLight {
    vec3  Position;
    vec3  Color;
    float Intensity;
};

layout (std140, set = 1, binding = 0) uniform SceneData {
    vec3 cameraPosition;
    DirectionalLight directionalLight;
    PointLight pointLights[10];
    int lightCount;
} u_SceneData;

layout (push_constant) uniform MeshData {
    mat4  u_ModelMatrix;
    vec3  u_Albedo;
    float u_Roughness;
    float u_Metallic;
};

vec3 CalculateDirectionalLight(vec3 normal, DirectionalLight light)
{
    float dotFactor = max(dot(light.Direction, normal), 0.0);
    vec3 diffuse = dotFactor * light.Color;
    return diffuse;
}

vec3 GetPixelColor()
{
    // return u_Albedo;
    return texture(u_TextureSampler, v_TextureCoords).xyz;
}

// PBR Lighting
vec3 SchlickFresnel(float v_dot_h)
{
    vec3 F0 = vec3(0.04);

    if (u_Metallic == 1.0) {
        F0 = GetPixelColor();
    }

    vec3 ret = F0 + (1 - F0) * pow(clamp(1.0 - v_dot_h, 0.0, 1.0), 5);
    return ret;
}


float GeomSmith(float dp)
{
    float k = (u_Roughness + 1.0) * (u_Roughness + 1.0) / 8.0;
    float denom = dp * (1 - k) + k;
    return dp / denom;
}


float GGXDistribution(float n_dot_h)
{
    float alpha2 = pow(u_Roughness, 4);
    float d = n_dot_h * n_dot_h * (alpha2 - 1) + 1;
    float ggxdistrib = alpha2 / (PI * d * d);
    return ggxdistrib;
}

vec3 CalculatePBRDirectionalLight(DirectionalLight light, vec3 normal)
{
    vec3 lightIntensity = light.Color * light.Intensity;
    
    vec3 l = normalize(-light.Direction);
    
    vec3 n = normal;
    vec3 v = normalize(u_SceneData.cameraPosition - v_Position);
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
    
    if (u_Metallic == 0.0) {
        fLambert = GetPixelColor();
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
    vec3 v = normalize(u_SceneData.cameraPosition - v_Position);
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
    
    if (u_Metallic == 0.0) {
        fLambert = GetPixelColor();
    }
    
    vec3 diffuseBRDF = kD * fLambert / PI;
    
    vec3 finalColor = (diffuseBRDF + specularBRDF) * lightIntensity * n_dot_l;
    return finalColor;
}

vec4 CalculatePBRLighting()
{
    vec3 normal = normalize(v_Normal);

    vec3 totalLight = vec3(0.0);

    totalLight += CalculatePBRDirectionalLight(u_SceneData.directionalLight, normal);

    for (int i = 0; i < u_SceneData.lightCount; i++)
        totalLight += CalculatePBRPointLight(u_SceneData.pointLights[i], normal);
    
    // HDR tone mapping
    totalLight = totalLight / (totalLight + vec3(1.0));
    
    // Gamma correction
    return vec4(pow(totalLight, vec3(1.0 / 2.2)), 1.0);
}

void main()
{
    FragColor = CalculatePBRLighting();
}
