#version 450

layout (location = 0) in vec4 v_VertexColor;
layout (location = 1) in vec3 v_Normal;
layout (location = 2) in vec2 v_TextureCoords;

layout (location = 0) out vec4 FragColor;

layout (binding = 1) uniform sampler2D u_TextureSampler;

void main()
{
    float ambient = 0.01;
    vec3 normal = normalize(v_Normal);

    vec3 lightColor = vec3(1.0);
    vec3 lightDirection = vec3(-1, -1, -1);
    float lightIntensity = max(dot(-lightDirection, normal), 0.0);
    vec3 diffuse = lightIntensity * lightColor;

    vec3 lighting = diffuse + ambient;

    FragColor = texture(u_TextureSampler, v_TextureCoords) * vec4(lighting, 1.0);
}
