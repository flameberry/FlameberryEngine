#version 450

layout (location = 0) out vec2 v_TextureUV;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	v_TextureUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(v_TextureUV * 2.0 - 1.0, 0.0, 1.0);
}
