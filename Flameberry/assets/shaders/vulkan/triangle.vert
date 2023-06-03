#version 450

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TextureCoords;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_BiTangent;

layout (location = 0) out vec3 v_Position;
layout (location = 1) out vec3 v_Normal;
layout (location = 2) out vec2 v_TextureCoords;
layout (location = 3) out vec4 v_LightFragmentPosition;
layout (location = 4) out mat3 v_TBNMatrix;

layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 u_ViewProjectionMatrix;
    mat4 u_LightViewProjectionMatrix;
};

layout (push_constant) uniform MeshData {
    mat4  u_ModelMatrix;
    vec3  u_Albdeo;
    float u_Roughness;
    bool  u_Metallic;
};

mat4 g_BiasMatrix = mat4(
    0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0
);

void main()
{
    gl_Position = u_ViewProjectionMatrix * u_ModelMatrix * vec4(a_Position, 1.0);

    v_Position = a_Position;
    v_Normal = normalize(a_Normal);
    v_TextureCoords = a_TextureCoords;

    mat3 worldSpaceMatrix = transpose(inverse(mat3(u_ModelMatrix)));

    v_Normal =  worldSpaceMatrix * v_Normal; // TODO: Currently inefficient
    v_Position = vec3(u_ModelMatrix * vec4(v_Position, 1.0));

    v_LightFragmentPosition = (g_BiasMatrix * u_LightViewProjectionMatrix) * vec4(a_Position, 1.0);

    vec3 T = normalize(vec3(u_ModelMatrix * vec4(a_Tangent, 0.0)));
    vec3 N = normalize(vec3(u_ModelMatrix * vec4(a_Normal,  0.0)));

    // Re-orthogonalize T with respect to N
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    if (dot(a_Normal, cross(a_Tangent, a_BiTangent)) < 0.0)
        B *= -1.0;

    v_TBNMatrix = mat3(T, B, N);
}