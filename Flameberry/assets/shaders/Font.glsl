#shader vertex
#version 410 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;
layout (location = 2) in vec2 a_Texture_UV;
layout (location = 3) in float a_TextureIndex;

out vec4 v_Color;
out vec2 v_Texture_UV;
out float v_TextureIndex;

layout (std140) uniform Camera
{
    mat4 ViewProjectionMatrix;
} u_Camera;

void main()
{
    v_Color = a_Color;
    v_Texture_UV = a_Texture_UV;
    v_TextureIndex = a_TextureIndex;

    gl_Position = u_Camera.ViewProjectionMatrix * vec4(a_Position, 1.0);
}

#shader fragment
#version 410 core

out vec4 FragColor;

uniform sampler2D u_TextureSamplers[16];
uniform float u_PixelRange;
uniform float u_Strength;

in vec4 v_Color;
in vec2 v_Texture_UV;
in float v_TextureIndex;

float median(float r, float g, float b) 
{
    return max(min(r, g), min(max(r, g), b));
}

void main()
{
    if (v_TextureIndex == -1.0)
    {
        FragColor = v_Color;
    }
    else
    {
        int index = int(v_TextureIndex);
        float threshold = 1.0 - u_Strength;
        vec2 msdfUnit = u_PixelRange/vec2(textureSize(u_TextureSamplers[index], 0));
        vec3 s = texture(u_TextureSamplers[index], v_Texture_UV).rgb;
        float signDist = median(s.r, s.g, s.b) - threshold;
        signDist *= dot(msdfUnit, 0.5/fwidth(v_Texture_UV));
        float opacity = clamp(signDist + 0.5, 0.0, 1.0);

        // FragColor = mix(vec4(0.0, 0.0, 0.0, 0.0), v_Color, opacity);
        FragColor = vec4(v_Color.rgb, v_Color.a * opacity);
    }
}
