#version 450

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;

layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 u_ViewMatrix, u_ProjectionMatrix, u_ViewProjectionMatrix;
};

layout (push_constant) uniform ModelMatrix {
    mat4 u_ModelMatrix;
    vec2 u_ScreenSize;
};

void main()
{
    gl_Position = u_ViewProjectionMatrix * u_ModelMatrix * vec4(a_Position, 1.0);

    // const float outlineWidth = 1.0;
    // gl_Position = u_ViewProjectionMatrix * u_ModelMatrix * vec4(a_Position + normalize(a_Normal) * outlineWidth, 1.0);
    // vec4 normal = u_ViewProjectionMatrix * u_ModelMatrix * vec4(a_Normal, 1.0);
    // gl_Position.xy += normalize(normal.xy) / u_ScreenSize.xy * gl_Position.w * outlineWidth * 2;
}