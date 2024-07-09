#version 450

layout(location = 0) in vec3 v_Color;
layout(location = 1) in vec2 v_TextureCoords;

layout(location = 0) out vec4 o_FragColor;

layout(set = 1, binding = 0) uniform sampler2D u_FontAtlasSampler;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

const float pxRange = 2.0f; // set to distance field's pixel range

float screenPxRange()
{
    vec2 unitRange = vec2(pxRange) / vec2(textureSize(u_FontAtlasSampler, 0));
    vec2 screenTexSize = vec2(1.0) / fwidth(v_TextureCoords);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

void main()
{
    vec3 msd = texture(u_FontAtlasSampler, v_TextureCoords).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float screenPxDistance = screenPxRange() * (sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);

    if (opacity == 0.0)
        discard;

    vec4 bgColor = vec4(vec3(0.0), 0.0);
    o_FragColor = mix(bgColor, vec4(v_Color, 1.0), opacity);
}
