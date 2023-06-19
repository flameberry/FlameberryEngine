#version 450

layout (location = 0) in vec3 v_Position;
layout (location = 1) in vec3 v_Normal;
layout (location = 2) in vec2 v_TextureCoords;
layout (location = 3) in vec4 v_LightFragmentPosition;
layout (location = 4) in mat3 v_TBNMatrix;

layout (location = 0) out vec4 o_FragColor;

#define PI 3.1415926535897932384626433832795
#define AMBIENT 0.1

layout (set = 1, binding = 1) uniform sampler2D u_ShadowMapSampler;

layout (set = 2, binding = 0) uniform sampler2D u_TextureMapSampler;
layout (set = 3, binding = 0) uniform sampler2D u_NormalMapSampler;
layout (set = 4, binding = 0) uniform sampler2D u_RoughnessMapSampler;
layout (set = 5, binding = 0) uniform sampler2D u_AmbientOcclusionMapSampler;
layout (set = 6, binding = 0) uniform sampler2D u_MetallicMapSampler;

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

    float u_TextureMapEnabled, u_NormalMapEnabled, u_RoughnessMapEnabled, u_AmbientOcclusionMapEnabled, u_MetallicMapEnabled;
};

vec3 CalculateDirectionalLight(vec3 normal, DirectionalLight light)
{
    float dotFactor = max(dot(light.Direction, normal), 0.0);
    vec3 diffuse = dotFactor * light.Color;
    return diffuse;
}

vec3 GetPixelColor()
{
    if (u_TextureMapEnabled == 1.0)
        return texture(u_TextureMapSampler, v_TextureCoords).xyz;
    return u_Albedo;
}

vec3 GetPixelNormal()
{
    if (u_NormalMapEnabled == 1.0)
    {
        vec3 rgbNormal = texture(u_NormalMapSampler, v_TextureCoords).rgb * 2.0 - 1.003921568627451;
        return normalize(v_TBNMatrix * normalize(rgbNormal));
    }
    return normalize(v_Normal);
}

float GetPixelRoughness()
{
    if (u_RoughnessMapEnabled == 1.0)
        return texture(u_RoughnessMapSampler, v_TextureCoords).x;
    return u_Roughness;
}

float GetAmbientOcclusion()
{
    if (u_AmbientOcclusionMapEnabled == 1.0)
        return texture(u_AmbientOcclusionMapSampler, v_TextureCoords).x;
    return 1.0;
}

float GetMetallicFactor()
{
    if (u_MetallicMapEnabled == 1.0)
        return texture(u_MetallicMapSampler, v_TextureCoords).x;
    return u_Metallic;
}

vec3 SchlickFresnel(float v_dot_h)
{
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, GetPixelColor(), GetMetallicFactor());
    return F0 + (1 - F0) * pow(clamp(1.0 - v_dot_h, 0.0, 1.0), 5.0);
}

float GeomSmith(float dp)
{
    float k = (GetPixelRoughness() + 1.0) * (GetPixelRoughness() + 1.0) / 8.0;
    float denom = dp * (1.0 - k) + k;
    return dp / denom;
}

float GGXDistribution(float n_dot_h)
{
    float alpha2 = pow(GetPixelRoughness(), 4);
    float d = n_dot_h * n_dot_h * (alpha2 - 1.0) + 1.0;
    float ggxdistrib = alpha2 / (PI * d * d);
    return ggxdistrib;
}

float CalculateShadowFactor()
{
    float shadow = 1.0;
    // perform perspective divide
    vec3 projCoords = v_LightFragmentPosition.xyz / v_LightFragmentPosition.w;

    if (projCoords.z < -1.0 || projCoords.z > 1.0)
        return AMBIENT;

    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(u_ShadowMapSampler, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    if (v_LightFragmentPosition.w > 0.0 && currentDepth - closestDepth > 0.0001)
        shadow = AMBIENT;
    return shadow;
}

float FilterPCF(/*float bias*/)
{
    ivec2 textureDimensions = textureSize(u_ShadowMapSampler, 0);
    float scale = 1.0;
    float texelWidth = scale * 1.0 / float(textureDimensions.x);
    float texelHeight = scale * 1.0 / float(textureDimensions.y);

    vec2 texelSize = vec2(texelWidth, texelHeight);
    vec3 u_ShadowMapSamplerCoords = v_LightFragmentPosition.xyz / v_LightFragmentPosition.w;

    float shadowFactor = 0.0;

    int range = 2;
    float filterSize = 0.0;
    // float filterSize = 9.0;

    for (int y = -range; y <= range; y++)
    {
        for (int x = -range; x <= range; x++)
        {
            vec2 offset = vec2(x, y) * texelSize;
            float depth = texture(u_ShadowMapSampler, u_ShadowMapSamplerCoords.xy + offset).r;

            if (depth /*+ bias*/ < u_ShadowMapSamplerCoords.z)
                shadowFactor += 0.0;
            else
                shadowFactor += 1.0;
            
            filterSize++;
        }
    }
    shadowFactor /= filterSize;
    return shadowFactor;
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
    kD *= 1.0 - GetMetallicFactor();
    
    vec3 specularBRDFNumerator = GGXDistribution(n_dot_h) * fresnelFactor * GeomSmith(n_dot_l) * GeomSmith(n_dot_v);
    float specularBRDFDenominator = 4.0 * n_dot_v * n_dot_l + 0.0001;
    
    vec3 specularBRDF = specularBRDFNumerator / specularBRDFDenominator;
    
    vec3 diffuseBRDF = kD * GetPixelColor() / PI;

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
    kD *= 1.0 - GetMetallicFactor();
    
    vec3 specularBRDFNumerator = GGXDistribution(n_dot_h) * fresnelFactor * GeomSmith(n_dot_l) * GeomSmith(n_dot_v);
    float specularBRDFDenominator = 4.0 * n_dot_v * n_dot_l + 0.0001;
    
    vec3 specularBRDF = specularBRDFNumerator / specularBRDFDenominator;
    
    vec3 diffuseBRDF = kD * GetPixelColor() / PI;
    
    vec3 finalColor = (diffuseBRDF + specularBRDF) * lightIntensity * n_dot_l;
    return finalColor;
}

vec3 CalculatePBRLighting(vec3 normal)
{
    vec3 totalLight = vec3(0.0);

    totalLight += CalculatePBRDirectionalLight(u_SceneData.directionalLight, normal);

    for (int i = 0; i < u_SceneData.lightCount; i++)
        totalLight += CalculatePBRPointLight(u_SceneData.pointLights[i], normal);
    
    vec3 ambient = AMBIENT * GetAmbientOcclusion() * GetPixelColor();
    return totalLight + ambient;
}

void main()
{
    vec3 normal = GetPixelNormal();
    vec3 intermediateColor = CalculatePBRLighting(normal);

    // HDR tone mapping
    intermediateColor = intermediateColor / (intermediateColor + vec3(1.0));

    o_FragColor = vec4(intermediateColor, 1.0);
}
