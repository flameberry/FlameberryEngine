#shader vertex
#version 410 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;
layout (location = 2) in vec2 a_Texture_UV;

out vec4 v_Color;
out vec2 v_Texture_UV;

uniform mat4 u_ViewProjectionMatrix;

void main()
{
    v_Color = a_Color;
    v_Texture_UV = a_Texture_UV;

    gl_Position = u_ViewProjectionMatrix * vec4(a_Position, 1.0);
}

#shader fragment
#version 410 core

out vec4 FragColor;

uniform sampler2D u_Texture;
uniform int u_Is_Texture_On;
uniform vec2 u_Resolution;

in vec4 v_Color;
in vec2 v_Texture_UV;

void main()
{
    // u_Resolution = vec2(1280.0, 720.0);

    vec2 uv = (gl_FragCoord.xy / u_Resolution - 0.5) * 2.0;

    float aspect = u_Resolution.x / u_Resolution.y;

    uv.x *= aspect;

    FragColor = vec4(uv, 1.0, 1.0);
    FragColor.b = 0.0;

    float distance = 1.0 - length(uv);

    float fade_factor = 0.005;

    vec3 col = vec3(smoothstep(0.0, fade_factor, distance));

    FragColor.rgb = col;
}
