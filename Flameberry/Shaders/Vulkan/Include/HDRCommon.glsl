vec3 ToneMap(vec3 color)
{
    return color / (color + 1.0f);
}

vec3 ToneMapWithExposure(vec3 color, float exposure)
{
    return vec3(1.0) - exp(-color * exposure);
}

// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
vec3 AcesFilmicToneMap(vec3 x)
{
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 AcesFilmicToneMapWithExposure(vec3 x, float exposure)
{
	x *= exposure;
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}
