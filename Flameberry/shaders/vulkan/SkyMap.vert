#version 450

layout (location = 0) out vec3 v_Position;
layout (location = 1) out vec2 v_TextureCoords;

layout (push_constant) uniform UniformBufferObject {
    mat4 u_ViewProjectionMatrix;
    float u_Exposure;
};

struct Vertex {
    vec3 Position;
    vec2 TextureCoords;
};

const Vertex vertices[] = {
    Vertex(vec3(-0.5f, -0.5f, -0.5f), vec2(0.0f, 0.0f)),  // A 0
    Vertex(vec3(0.5f, -0.5f, -0.5f),  vec2(1.0f, 0.0f)),  // B 1
    Vertex(vec3(0.5f,  0.5f, -0.5f),  vec2(1.0f, 1.0f)),  // C 2
    Vertex(vec3(-0.5f,  0.5f, -0.5f), vec2(0.0f, 1.0f)),  // D 3
    Vertex(vec3(-0.5f, -0.5f,  0.5f), vec2(0.0f, 0.0f)),  // E 4
    Vertex(vec3(0.5f, -0.5f,  0.5f),  vec2(1.0f, 0.0f)),  // F 5
    Vertex(vec3(0.5f,  0.5f,  0.5f),  vec2(1.0f, 1.0f)),  // G 6
    Vertex(vec3(-0.5f,  0.5f,  0.5f), vec2(0.0f, 1.0f)),  // H 7

    Vertex(vec3(-0.5f,  0.5f, -0.5f), vec2(0.0f, 0.0f)),  // D 8
    Vertex(vec3(-0.5f, -0.5f, -0.5f), vec2(1.0f, 0.0f)),  // A 9
    Vertex(vec3(-0.5f, -0.5f,  0.5f), vec2(1.0f, 1.0f)),  // E 10
    Vertex(vec3(-0.5f,  0.5f,  0.5f), vec2(0.0f, 1.0f)),  // H 11
    Vertex(vec3(0.5f, -0.5f, -0.5f),  vec2(0.0f, 0.0f)),  // B 12
    Vertex(vec3(0.5f,  0.5f, -0.5f),  vec2(1.0f, 0.0f)),  // C 13
    Vertex(vec3(0.5f,  0.5f,  0.5f),  vec2(1.0f, 1.0f)),  // G 14
    Vertex(vec3(0.5f, -0.5f,  0.5f),  vec2(0.0f, 1.0f)),  // F 15

    Vertex(vec3(-0.5f, -0.5f, -0.5f), vec2(0.0f, 0.0f)),  // A 16
    Vertex(vec3(0.5f, -0.5f, -0.5f),  vec2(1.0f, 0.0f)),  // B 17
    Vertex(vec3(0.5f, -0.5f,  0.5f),  vec2(1.0f, 1.0f)),  // F 18
    Vertex(vec3(-0.5f, -0.5f,  0.5f), vec2(0.0f, 1.0f)),  // E 19
    Vertex(vec3(0.5f,  0.5f, -0.5f),  vec2(0.0f, 0.0f)),  // C 20
    Vertex(vec3(-0.5f,  0.5f, -0.5f), vec2(1.0f, 0.0f)),  // D 21
    Vertex(vec3(-0.5f,  0.5f,  0.5f), vec2(1.0f, 1.0f)),  // H 22
    Vertex(vec3(0.5f,  0.5f,  0.5f),  vec2(0.0f, 1.0f))   // G 23
};

const uint indices[] = {
    // front and back
    0, 3, 2,
    2, 1, 0,
    4, 5, 6,
    6, 7 ,4,
    // left and right
    11, 8, 9,
    9, 10, 11,
    12, 13, 14,
    14, 15, 12,
    // bottom and top
    16, 17, 18,
    18, 19, 16,
    20, 21, 22,
    22, 23, 20
};

void main()
{
    v_Position = vertices[indices[gl_VertexIndex]].Position;
    v_TextureCoords = vertices[indices[gl_VertexIndex]].TextureCoords;

    vec4 position = u_ViewProjectionMatrix * vec4(v_Position, 1.0);
    gl_Position = position.xyww;
}
