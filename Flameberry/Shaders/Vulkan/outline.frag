#version 450

layout (location = 0) out vec4 o_FragColor;

void main()
{
    const vec4 color = vec4(254.0 / 255.0, 211.0 / 255.0, 140.0 / 255.0, 1.0);
    o_FragColor = color;
}