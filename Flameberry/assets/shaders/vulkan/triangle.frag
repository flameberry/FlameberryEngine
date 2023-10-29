#version 450

layout (location = 0) in vec3 v_WorldSpacePosition;
layout (location = 1) in vec3 v_ClipSpacePosition;
layout (location = 2) in vec3 v_Normal;
layout (location = 3) in vec2 v_TextureCoords;
layout (location = 4) in vec3 v_ViewSpacePosition;
layout (location = 5) in mat3 v_TBNMatrix;

layout (location = 0) out vec4 o_FragColor;

#define PI 3.1415926535897932384626433832795
#define AMBIENT 0.2f
#define CASCADE_COUNT 4

#extension GL_GOOGLE_include_directive : enable

// #define POISSON_PROVIDE_32_SAMPLES
// #include "include/poisson.glsl"

const mat4 g_BiasMatrix = mat4(
    0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0
);

layout (set = 1, binding = 1) uniform sampler2DArray u_ShadowMapSamplerArray;

layout (set = 2, binding = 0) uniform sampler2D u_TextureMapSampler;
layout (set = 2, binding = 1) uniform sampler2D u_NormalMapSampler;
layout (set = 2, binding = 2) uniform sampler2D u_RoughnessMapSampler;
layout (set = 2, binding = 3) uniform sampler2D u_AmbientOcclusionMapSampler;
layout (set = 2, binding = 4) uniform sampler2D u_MetallicMapSampler;

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

struct SceneRendererSettingsUniform {
    int EnableShadows, ShowCascades, SoftShadows;
};

layout (std140, set = 1, binding = 0) uniform SceneData {
    mat4 u_CascadeMatrices[CASCADE_COUNT];
    vec4 u_CascadeDepthSplits;
    vec3 u_CameraPosition;
    DirectionalLight u_DirectionalLight;
    PointLight u_PointLights[10];
    int u_LightCount;
    SceneRendererSettingsUniform u_SceneRendererSettings;
};

layout (push_constant) uniform MeshData {
    mat4  u_ModelMatrix;
    vec3  u_Albedo;
    float u_Roughness;
    float u_Metallic;

    float u_TextureMapEnabled, u_NormalMapEnabled, u_RoughnessMapEnabled, u_AmbientOcclusionMapEnabled, u_MetallicMapEnabled;
};

vec3 GetPixelColor()
{
    if (u_TextureMapEnabled == 1.0f)
        return texture(u_TextureMapSampler, v_TextureCoords).xyz;
    return u_Albedo;
}

vec3 GetPixelNormal()
{
    if (u_NormalMapEnabled == 1.0f)
    {
        vec3 rgbNormal = texture(u_NormalMapSampler, v_TextureCoords).rgb * 2.0f - 1.003921568627451f;
        return normalize(v_TBNMatrix * normalize(rgbNormal));
    }
    return normalize(v_Normal);
}

float GetPixelRoughness()
{
    if (u_RoughnessMapEnabled == 1.0f)
        return texture(u_RoughnessMapSampler, v_TextureCoords).x;
    return u_Roughness;
}

float GetAmbientOcclusion()
{
    if (u_AmbientOcclusionMapEnabled == 1.0f)
        return texture(u_AmbientOcclusionMapSampler, v_TextureCoords).x;
    return 1.0;
}

float GetMetallicFactor()
{
    if (u_MetallicMapEnabled == 1.0f)
        return texture(u_MetallicMapSampler, v_TextureCoords).x;
    return u_Metallic;
}

vec3 SchlickFresnel(float v_dot_h)
{
    vec3 F0 = vec3(0.04f);
    F0 = mix(F0, GetPixelColor(), GetMetallicFactor());
    return F0 + (1 - F0) * pow(clamp(1.0f - v_dot_h, 0.0f, 1.0f), 5.0f);
}

float GeomSmith(float dp)
{
    float k = (GetPixelRoughness() + 1.0f) * (GetPixelRoughness() + 1.0f) / 8.0f;
    float denom = dp * (1.0f - k) + k;
    return dp / denom;
}

float GGXDistribution(float n_dot_h)
{
    float alpha2 = pow(GetPixelRoughness(), 4);
    float d = n_dot_h * n_dot_h * (alpha2 - 1.0f) + 1.0f;
    float ggxdistrib = alpha2 / (PI * d * d);
    return ggxdistrib;
}

vec2 VogelDiskSample(uint sampleIndex, uint sampleCount, float phi)
{
    const float goldenAngle = 2.4f;

    const float r = sqrt(sampleIndex + 0.5f) / sqrt(sampleCount);
    const float theta = sampleIndex * goldenAngle + phi;

    const float sine = sin(theta), cosine = cos(theta);
    return vec2(r * cosine, r * sine);
}

float InterleavedGradientNoise(vec2 position)
{
    vec3 magic = vec3(0.06711056f, 0.00583715f, 52.9829189f);
    return fract(magic.z * fract(dot(position, magic.xy)));
}

float Noise(vec2 position)
{
	const vec3 magic = vec3(12.9898f, 78.233f, 43758.5453123f);
    return fract(magic.z * sin(dot(position.xy, magic.xy)));
}

float TextureProj_DirectionalLight(vec4 shadowCoord, vec2 offset, uint cascadeIndex, float bias)
{
	float shadow = 1.0f;
	if (shadowCoord.z > -1.0f && shadowCoord.z < 1.0f) {
		float dist = texture(u_ShadowMapSamplerArray, vec3(shadowCoord.st + offset, cascadeIndex)).r;
		if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) {
			// shadow = AMBIENT * GetAmbientOcclusion();
            shadow = 0.0f;
		}
	}
	return shadow;
}

float FilterPCF_DirectionalLight(vec4 sc, uint cascadeIndex, float pixelSize, int filterSize, float bias)
{
    const ivec2 texDim = textureSize(u_ShadowMapSamplerArray, 0).xy;
	const float dx = pixelSize / float(texDim.x);
	const float dy = pixelSize / float(texDim.y);

	float shadowFactor = 0.0f;
	for (int x = -filterSize; x <= filterSize; x++) {
		for (int y = -filterSize; y <= filterSize; y++) {
			shadowFactor += TextureProj_DirectionalLight(sc, vec2(dx * x, dy * y), cascadeIndex, bias);
		}
	}
	const int count = 4 * filterSize + 2;
	return shadowFactor / count;
}

float FilterPCFRadial_DirectionalLight(vec4 sc, uint cascadeIndex, float radius, uint sampleCount, float bias, float interleavedNoise)
{
    const float scale = 1.0f + u_CascadeDepthSplits[cascadeIndex] / 1000.01f;
    const vec2 texelSize = scale / textureSize(u_ShadowMapSamplerArray, 0).xy;

    float shadowFactor = 0.0f;
	for (uint i = 0; i < sampleCount; i++) 
    {
        // vec2 offset = vec2(g_PoissonSamples[i]);
        vec2 offset = VogelDiskSample(i, sampleCount, interleavedNoise);

        shadowFactor += TextureProj_DirectionalLight(sc, radius * texelSize * offset, cascadeIndex, bias);
	}
	return shadowFactor / sampleCount;
}

float PCSS_SearchWidth(float lightSize, float receiverDistance, uint cascadeIndex)
{
	return lightSize * (receiverDistance + u_CascadeDepthSplits[cascadeIndex] / 1000.0f) / receiverDistance;
}

vec2 PCSS_BlockerDistance(vec3 projCoords, float searchUV, uint cascadeIndex, float interleavedNoise, float bias)
{
    const uint blockerSearch_SampleCount = 16;

    // Perform N samples with pre-defined offset and random rotation, scale by input search size
	int blockers = 0;
	float avgBlockerDepth = 0.0f;

    const float scale = 1.0f + u_CascadeDepthSplits[cascadeIndex] / 1000.01f;
    const vec2 texelSize = scale / textureSize(u_ShadowMapSamplerArray, 0).xy;

    for (uint i = 0; i < blockerSearch_SampleCount; i++)
    {
        // vec2 offset = vec2(g_PoissonSamples[i]) * searchUV * texelSize;
        vec2 offset = VogelDiskSample(i, blockerSearch_SampleCount, interleavedNoise) * searchUV * texelSize;
        float randomlySampledZ = texture(u_ShadowMapSamplerArray, vec3(projCoords.xy + offset, cascadeIndex)).r;

        if (randomlySampledZ < projCoords.z - bias)
		{
			blockers++;
			avgBlockerDepth += randomlySampledZ;
		}
    }

    avgBlockerDepth /= blockers;
    return vec2(avgBlockerDepth, blockers);
}

float PCSS_Shadow_DirectionalLight(vec4 shadowCoord, uint cascadeIndex, float interleavedNoise, float bias)
{
    const float lightSize = 20.0f;

    float receiverDepth = shadowCoord.z;
    float searchWidth = PCSS_SearchWidth(lightSize, receiverDepth, cascadeIndex);
    const vec2 blockerInfo = PCSS_BlockerDistance(shadowCoord.xyz, searchWidth, cascadeIndex, interleavedNoise, bias);
    
    if (blockerInfo.y == 0.0f)
        return 1.0f;

    float penumbraSize = receiverDepth - blockerInfo.x;
    if (penumbraSize == 0.0f)
        return 1.0f;
    
    // float softnessFallOff = 2.0f;
    // penumbraSize = 1.0 - pow(1.0 - penumbraSize, softnessFallOff);

    float filterRadius = penumbraSize * lightSize;
    return FilterPCFRadial_DirectionalLight(shadowCoord, cascadeIndex, filterRadius, 16, bias, interleavedNoise);
}

vec3 PBR_DirectionalLight(DirectionalLight light, vec3 normal)
{
    vec3 lightIntensity = light.Color * light.Intensity;
    
    vec3 l = normalize(-light.Direction);
    
    vec3 n = normal;
    vec3 v = normalize(u_CameraPosition - v_WorldSpacePosition);
    vec3 h = normalize(v + l);
    
    float n_dot_h = max(dot(n, h), 0.0f);
    float v_dot_h = max(dot(v, h), 0.0f);
    float n_dot_l = max(dot(n, l), 0.0f);
    float n_dot_v = max(dot(n, v), 0.0f);
    
    vec3 fresnelFactor = SchlickFresnel(v_dot_h);
    vec3 kS = fresnelFactor;
    vec3 kD = 1.0f - kS;
    kD *= 1.0f - GetMetallicFactor();
    
    vec3 specularBRDFNumerator = GGXDistribution(n_dot_h) * fresnelFactor * GeomSmith(n_dot_l) * GeomSmith(n_dot_v);
    float specularBRDFDenominator = 4.0f * n_dot_v * n_dot_l + 0.0001f;
    
    vec3 specularBRDF = specularBRDFNumerator / specularBRDFDenominator;
    vec3 diffuseBRDF = kD * GetPixelColor() / PI;

    float shadow = 1.0f;
    if (u_SceneRendererSettings.EnableShadows == 1)
    {
        // Get cascade index for the current fragment's view position
        uint cascadeIndex = 0;
        for (uint i = 0; i < CASCADE_COUNT - 1; i++) {
            if (v_ViewSpacePosition.z < u_CascadeDepthSplits[i]) {	
                cascadeIndex = i + 1;
            }
        }

        float bias = max(0.05f * (1.0f - dot(normal, -l)), 0.002f);
        const float biasModifier = 0.5f;
        bias *= 1.0f / (-u_CascadeDepthSplits[cascadeIndex] * biasModifier);

        // Depth compare for shadowing
        vec4 shadowCoord = (g_BiasMatrix * u_CascadeMatrices[cascadeIndex]) * vec4(v_WorldSpacePosition, 1.0f);	

        if (u_SceneRendererSettings.SoftShadows == 1) {
            float interleavedNoise = 2.0 * PI * Noise(v_ClipSpacePosition.xy);
            shadow = PCSS_Shadow_DirectionalLight(shadowCoord / shadowCoord.w, cascadeIndex, interleavedNoise, bias);

            // shadow = FilterPCFRadial_DirectionalLight(shadowCoord / shadowCoord.w, cascadeIndex, 5.0f, 16, bias);
        }
        else
            shadow = TextureProj_DirectionalLight(shadowCoord / shadowCoord.w, vec2(0.0), cascadeIndex, bias);
    }

    vec3 finalColor = shadow * (diffuseBRDF + specularBRDF) * lightIntensity * n_dot_l;
    return finalColor;
}

vec3 PBR_PointLight(PointLight light, vec3 normal)
{
    vec3 lightIntensity = light.Color.xyz * light.Intensity;
    
    vec3 l = vec3(0.0f);
    l = light.Position - v_WorldSpacePosition;
    float lightToPixelDistance = length(l);
    l = normalize(l);
    lightIntensity /= lightToPixelDistance * lightToPixelDistance;
    
    vec3 n = normal;
    vec3 v = normalize(u_CameraPosition - v_WorldSpacePosition);
    vec3 h = normalize(v + l);
    
    float n_dot_h = max(dot(n, h), 0.0f);
    float v_dot_h = max(dot(v, h), 0.0f);
    float n_dot_l = max(dot(n, l), 0.0f);
    float n_dot_v = max(dot(n, v), 0.0f);
    
    vec3 fresnelFactor = SchlickFresnel(v_dot_h);
    vec3 kS = fresnelFactor;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - GetMetallicFactor();
    
    vec3 specularBRDFNumerator = GGXDistribution(n_dot_h) * fresnelFactor * GeomSmith(n_dot_l) * GeomSmith(n_dot_v);
    float specularBRDFDenominator = 4.0f * n_dot_v * n_dot_l + 0.0001f;
    
    vec3 specularBRDF = specularBRDFNumerator / specularBRDFDenominator;
    
    vec3 diffuseBRDF = kD * GetPixelColor() / PI;
    
    vec3 finalColor = (diffuseBRDF + specularBRDF) * lightIntensity * n_dot_l;
    return finalColor;
}

vec3 PBR_TotalLight(vec3 normal)
{
    vec3 totalLight = vec3(0.0f);

    totalLight += PBR_DirectionalLight(u_DirectionalLight, normal);

    for (int i = 0; i < u_LightCount; i++)
        totalLight += PBR_PointLight(u_PointLights[i], normal);
    
    const vec3 ambient = AMBIENT * GetAmbientOcclusion() * GetPixelColor();
    return totalLight + ambient;
}

void main()
{
    vec3 normal = GetPixelNormal();
    vec3 intermediateColor = PBR_TotalLight(normal);

    // HDR tone mapping
    intermediateColor = intermediateColor / (intermediateColor + vec3(1.0f));

    o_FragColor = vec4(intermediateColor, 1.0f);

    if (u_SceneRendererSettings.ShowCascades == 1)
    {
        uint cascadeIndex = 0;
        for(uint i = 0; i < CASCADE_COUNT - 1; ++i) {
            if (v_ViewSpacePosition.z < u_CascadeDepthSplits[i]) {	
                cascadeIndex = i + 1;
            }
        }

        switch(cascadeIndex) {
            case 0 : 
                o_FragColor.rgb *= vec3(1.0f, 0.25f, 0.25f);
                break;
            case 1 : 
                o_FragColor.rgb *= vec3(0.25f, 1.0f, 0.25f);
                break;
            case 2 : 
                o_FragColor.rgb *= vec3(0.25f, 0.25f, 1.0f);
                break;
            case 3 : 
                o_FragColor.rgb *= vec3(1.0f, 1.0f, 0.25f);
                break;
        }
    }
}
