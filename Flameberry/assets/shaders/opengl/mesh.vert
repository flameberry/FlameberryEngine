#version 410 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;
layout (location = 2) in vec3 a_Normal;
layout (location = 3) in vec2 a_TextureUV;

layout (location = 0) out vec3 v_Position;
layout (location = 1) out vec4 v_Color;
layout (location = 2) out vec3 v_Normal;
layout (location = 3) out vec2 v_TextureUV;
layout (location = 5) out vec4 v_LightFragmentPosition;

uniform mat4 u_ModelMatrix;

layout (std140) uniform Camera
{
    mat4 u_ViewProjectionMatrix;
    mat4 u_LightViewProjectionMatrix;
};

const mat4 g_BiasMatrix = mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.5, 0.5, 0.5, 1.0
);

void main()
{
    v_Color = a_Color;
    v_Normal = a_Normal;
    v_TextureUV = a_TextureUV;

    v_Position = vec3(u_ModelMatrix * vec4(a_Position, 1.0));

    gl_Position = u_ViewProjectionMatrix * vec4(v_Position, 1.0);

    v_Normal = mat3(transpose(inverse(u_ModelMatrix))) * v_Normal;
    v_LightFragmentPosition = g_BiasMatrix * u_LightViewProjectionMatrix * vec4(v_Position, 1.0);
}