#version 450

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;
layout (location = 2) in vec3 a_Normal;
layout (location = 3) in vec2 a_TextureCoords;

layout (location = 0) out vec3 v_Position;
layout (location = 1) out vec4 v_VertexColor;
layout (location = 2) out vec3 v_Normal;
layout (location = 3) out vec2 v_TextureCoords;

layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 ViewProjectionMatrix;
} u_MVP_Data;

layout (push_constant) uniform MeshData {
    mat4  u_ModelMatrix;
    vec3  u_Albdeo;
    float u_Roughness;
    bool  u_Metallic;
};

void main()
{
    gl_Position = u_MVP_Data.ViewProjectionMatrix * u_ModelMatrix * vec4(a_Position, 1.0);

    v_Position = a_Position;
    v_VertexColor = a_Color;
    v_Normal = a_Normal;
    v_TextureCoords = a_TextureCoords;

    v_Normal = mat3(transpose(inverse(u_ModelMatrix))) * v_Normal; // TODO: Currently inefficient
    v_Position = vec3(u_ModelMatrix * vec4(v_Position, 1.0));
}