#version 450

layout(location = 0) out vec3 nearPoint;
layout(location = 1) out vec3 farPoint;
layout(location = 2) out mat4 v_ViewMatrix;
layout(location = 6) out mat4 v_ProjectionMatrix;
layout(location = 10) out vec2 v_TextureCoords;

// These are the uniforms that are set by Renderer and are not exposed to the Material class
// How do we decide that? The classes which are marked by _FBY_ prefix are considered Renderer only
layout(set = 0, binding = 0) uniform _FBY_CameraMatrices {
    mat4 u_ViewMatrix, u_ProjectionMatrix, u_ViewProjectionMatrix;
};

// Grid position are in clipped space
vec3 gridPlane[6] = vec3[](
        vec3(1, 1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
        vec3(-1, -1, 0), vec3(1, 1, 0), vec3(1, -1, 0)
    );

vec2 gridUV[6] = vec2[](
        vec2(1, 1), vec2(0, 0), vec2(0, 1),
        vec2(0, 0), vec2(1, 1), vec2(1, 0)
    );

vec3 UnprojectPoint(float x, float y, float z, mat4 view, mat4 projection)
{
    mat4 viewInv = inverse(u_ViewMatrix);
    mat4 projInv = inverse(u_ProjectionMatrix);
    vec4 unprojectedPoint = viewInv * projInv * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main()
{
    vec3 p = gridPlane[gl_VertexIndex].xyz;
    nearPoint = UnprojectPoint(p.x, p.y, 0.0, u_ViewMatrix, u_ProjectionMatrix).xyz; // unprojecting on the near plane
    farPoint = UnprojectPoint(p.x, p.y, 1.0, u_ViewMatrix, u_ProjectionMatrix).xyz; // unprojecting on the far plane
    gl_Position = vec4(p, 1.0); // using directly the clipped coordinates

    v_TextureCoords = gridUV[gl_VertexIndex];

    v_ViewMatrix = u_ViewMatrix;
    v_ProjectionMatrix = u_ProjectionMatrix;
}
