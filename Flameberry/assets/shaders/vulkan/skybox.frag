#version 450

layout (location = 0) in vec3 v_Position;
layout (location = 1) in vec2 v_TextureCoords;

layout (location = 0) out vec4 o_FragColor;

layout (set = 0, binding = 0) uniform sampler2D u_EquirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 textureUV = vec2(atan(v.z, v.x), asin(v.y));
    textureUV *= invAtan;
    textureUV += 0.5;
    return vec2(textureUV.x, 1.0 - textureUV.y); // flip y
}

void main()
{
    vec3 normal = normalize(v_Position);
    vec2 textureUV = SampleSphericalMap(normal);
    vec3 color = texture(u_EquirectangularMap, textureUV).rgb;

    // Tone Mapping
    color = color / (color + vec3(1.0));

    const float gamma = 2.2;
    color = pow(color, vec3(1.0 / gamma));
    
    o_FragColor = vec4(color, 1.0);
}