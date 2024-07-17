#version 450

layout(location = 0) in vec3 v_Color;
layout(location = 1) in vec2 v_TextureCoords;

layout(location = 0) out vec4 o_FragColor;

layout(set = 1, binding = 0) uniform sampler2D u_TextureMapSampler;

void main()
{
    o_FragColor = texture(u_TextureMapSampler, v_TextureCoords);

    // Quick Fix to avoid sorting quads based on distance from camera: Change in future
    if (o_FragColor.a < 0.4)
        discard;
}
