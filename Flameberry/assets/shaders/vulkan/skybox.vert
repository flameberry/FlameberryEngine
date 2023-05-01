#version 450

layout (location = 0) in vec3 a_Position;

// vec3 skyboxVertices[] = {
//     //   Coordinates
//     vec3(-1.0, -1.0,  1.0),//        7--------6
//     vec3( 1.0, -1.0,  1.0),//       /|       /|
//     vec3( 1.0, -1.0, -1.0),//      4--------5 |
//     vec3(-1.0, -1.0, -1.0),//      | |      | |
//     vec3(-1.0,  1.0,  1.0),//      | 3------|-2
//     vec3( 1.0,  1.0,  1.0),//      |/       |/
//     vec3( 1.0,  1.0, -1.0),//      0--------1
//     vec3(-1.0,  1.0, -1.0)
// };

// int skyboxIndices[] = {
//     // Right
//     1, 2, 6,
//     6, 5, 1,
//     // Left
//     0, 4, 7,
//     7, 3, 0,
//     // Top
//     4, 5, 6,
//     6, 7, 4,
//     // Bottom
//     0, 3, 2,
//     2, 1, 0,
//     // Back
//     0, 1, 5,
//     5, 4, 0,
//     // Front
//     3, 7, 6,
//     6, 2, 3
// };

layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 u_ViewProjectionMatrix;
    mat4 u_LightViewProjectionMatrix;
};

void main()
{
    // vec4 position = u_ViewProjectionMatrix * vec4(skyboxVertices[skyboxIndices[gl_VertexIndex]], 1.0);
    vec4 position = u_ViewProjectionMatrix * vec4(a_Position, 1.0);
    // gl_Position = vec4(position.x, position.y, position.w, position.w);
    gl_Position = position;
}