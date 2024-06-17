#define PI 3.1415926535897932384626433832795

vec3 NormalizedToWorldPosition(int face, vec2 uv)
{
    switch (face)
    {
        case 0: return vec3( 1.0f,  uv.y,    -uv.x);
        case 1: return vec3(-1.0f,  uv.y,     uv.x);
        case 2: return vec3(+uv.x, -1.0f,    +uv.y);
        case 3: return vec3(+uv.x,  1.0f,    -uv.y);
        case 4: return vec3(+uv.x,  uv.y,     1.0f);
        case 5: return vec3(-uv.x, +uv.y,    -1.0f);
        default: return vec3(0.0f);
    }
}

vec2 DirToUV(vec3 dir)
{
	return vec2(
		0.5f + 0.5f * atan(dir.z, dir.x) / PI,
		1.f - acos(dir.y) / PI
    );
}

vec3 ToneMap(vec3 color)
{
    // Tone Mapping
    vec3 newColor = color / (color + 1.0f);
    return newColor;
}