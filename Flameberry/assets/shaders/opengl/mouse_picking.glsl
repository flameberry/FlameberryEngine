#shader vertex
#version 410 core
layout (location = 0) in vec3 a_Position;
layout (location = 4) in int a_EntityID;

layout (location = 0) flat out int v_EntityID;

uniform mat4 u_ModelMatrix;

layout (std140) uniform Camera {
    mat4 u_ViewProjectionMatrix;
};

void main()
{
    v_EntityID = a_EntityID;
    gl_Position = u_ViewProjectionMatrix * u_ModelMatrix * vec4(a_Position, 1.0);
}

#shader fragment
#version 410 core
layout (location = 0) out int o_EntityID;

layout (location = 0) flat in int v_EntityID;

void main()
{
    o_EntityID = v_EntityID;
}