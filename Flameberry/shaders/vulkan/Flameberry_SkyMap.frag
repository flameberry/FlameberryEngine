#version 450

layout (location = 0) in vec3 v_Position;
layout (location = 1) in vec2 v_TextureCoords;

layout (location = 0) out vec4 o_FragColor;

// These are the uniforms that are set by Renderer and are not exposed to the Material class
// How do we decide that? The classes which are marked by _FBY_ prefix are considered Renderer only
layout (set = 0, binding = 0) uniform samplerCube _FBY_u_Skymap;

// These are not used in this shader but present so that Flameberry can generate the correct descriptor set layout containing all bindings
layout (set = 0, binding = 1) uniform samplerCube _FBY_u_IrradianceMap;
layout (set = 0, binding = 2) uniform samplerCube _FBY_u_PrefilteredMap;
layout (set = 0, binding = 3) uniform sampler2D   _FBY_u_BRDFLUTMap;

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
