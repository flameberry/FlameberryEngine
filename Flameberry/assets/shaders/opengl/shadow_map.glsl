#shader vertex
#version 410 core

layout (location = 0) in vec3 a_Position;

layout (std140) uniform Camera
{
    mat4 u_ViewProjectionMatrix;
};

uniform mat4 u_ModelMatrix;

void main()
{
    gl_Position = u_ViewProjectionMatrix * u_ModelMatrix * vec4(a_Position, 1.0);
}

#shader fragment
#version 410 core

layout (location = 0) out vec4 FragColor;

void main() {}