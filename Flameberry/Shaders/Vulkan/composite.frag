#version 450

layout (location = 0) out vec4 o_FragColor;

layout (location = 0) in vec2 v_TextureUV;

layout (set = 0, binding = 0) uniform sampler2D u_ColorSampler;
layout (set = 0, binding = 1) uniform usampler2DMS u_StencilSampler;

void main()
{
    float weight[5];
	weight[0] = 0.227027;
	weight[1] = 0.1945946;
	weight[2] = 0.1216216;
	weight[3] = 0.054054;
	weight[4] = 0.016216;

	vec2 tex_offset = 1.0 / textureSize(u_ColorSampler, 0) * 2.0; // gets size of single texel
	vec3 result = texture(u_ColorSampler, v_TextureUV).rgb * weight[0]; // current fragment's contribution
	for(int i = 1; i < 5; ++i)
	{
        result += texture(u_ColorSampler, v_TextureUV + vec2(tex_offset.x * i, 0.0)).rgb * weight[i] * 1.0;
        result += texture(u_ColorSampler, v_TextureUV - vec2(tex_offset.x * i, 0.0)).rgb * weight[i] * 1.0;
        // result += texture(u_ColorSampler, v_TextureUV + vec2(0.0, tex_offset.y * i)).rgb * weight[i] * 10.0;
        // result += texture(u_ColorSampler, v_TextureUV - vec2(0.0, tex_offset.y * i)).rgb * weight[i] * 10.0;
	}
    o_FragColor = vec4(result, 1.0);

	// const ivec2 size = textureSize(u_StencilSampler);
 	// const ivec2 pixel_coords = ivec2(v_TextureUV) * size;
	// uvec4 stencil = texelFetch(u_StencilSampler, pixel_coords, 0);
	// // float stencil_f = float(stencil.r);
	// vec4 stencil_f = vec4(stencil);
	// // o_FragColor = vec4(stencil_f, 0.0, 0.0, 1.0);
	// o_FragColor = stencil_f;
	// o_FragColor.a = 1.0;
}
