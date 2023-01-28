#shader vertex
#version 410 core

layout (location = 0) in vec3 a_Position;

out vec3 v_TextureUV;

uniform mat4 u_ViewProjectionMatrix;

void main()
{
    vec4 position = u_ViewProjectionMatrix * vec4(a_Position, 1.0);
    gl_Position = vec4(position.x, position.y, position.w, position.w);

    v_TextureUV = vec3(a_Position.x, a_Position.y, -a_Position.z);
}

#shader fragment
#version 410 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out int o_EntityID;

in vec3 v_TextureUV;

uniform samplerCube u_SamplerCube;

void main()
{
    o_EntityID = -1;
    FragColor = texture(u_SamplerCube, v_TextureUV);
}
