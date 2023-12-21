#version 450

layout (location = 0) in vec3 v_Position;
layout (location = 1) in vec2 v_TextureCoords;

layout (location = 0) out vec4 o_FragColor;

layout (set = 0, binding = 0) uniform sampler2D u_EquirectangularMap;

const vec2 invAtan = vec2(0.1591f, 0.3183f);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 textureUV = vec2(atan(v.z, v.x), asin(v.y));
    textureUV *= invAtan;
    textureUV += 0.5f;
    return vec2(textureUV.x, 1.0f - textureUV.y); // flip y
}

void main()
{
    vec3 normal = normalize(v_Position);
    vec2 textureUV = SampleSphericalMap(normal);
    vec3 color = texture(u_EquirectangularMap, textureUV).rgb;

    // Tone Mapping
    color = color / (color + vec3(1.0f));

    const float gamma = 2.2f;
    color = pow(color, vec3(1.0f / gamma));
    
    o_FragColor = vec4(color, 1.0f);
}
