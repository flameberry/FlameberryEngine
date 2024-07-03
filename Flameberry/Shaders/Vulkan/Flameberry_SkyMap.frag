#version 450

layout (location = 0) in vec3 v_Position;
layout (location = 1) in vec2 v_TextureCoords;

layout (location = 0) out vec4 o_FragColor;

layout (set = 0, binding = 0) uniform samplerCube _FBY_u_Skymap;

void main()
{
    // Directly sample from the skymap which happens to be a cubemap :D
    vec3 color = texture(_FBY_u_Skymap, v_Position).rgb;

    // Tone Mapping
    color = color / (color + vec3(1.0f));

    const float gamma = 2.2f;
    color = pow(color, vec3(1.0f / gamma));
    
    o_FragColor = vec4(color, 1.0f);
}
