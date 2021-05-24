#version 460 core
out vec4 fragment_colour;
  
in vec2 uvs;
uniform sampler2D full_screen;
uniform float exposure_factor;

vec3 aces(vec3 x) 
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d ) + e), 0.0, 1.0);
}

vec3 tone_map(vec3 x) 
{
    return aces(x);
}

vec3 apply_exposure(vec3 colour)
{
    return 1 - exp(-colour*exposure_factor);
}

vec3 apply_gamma(vec3 colour)
{
	vec3 res = colour;
    if (res.r > 0.0031308)
	{
		res.r = 1.055 * pow(res.r, 1/2.4) - 0.055;
	}
	else
	{
		res.r = 12.92 * res.r;
	}

	if (res.g > 0.0031308)
	{
		res.g = 1.055 * pow(res.g, 1/2.4) - 0.055;
	}
	else
	{
		res.g = 12.92 * res.g;
	}

	if (res.b > 0.0031308)
	{
		res.b = 1.055 * pow(res.b, 1/2.4) - 0.055;
	}
	else
	{
		res.b = 12.92 * res.b;
	}

	return res;
}

void main()
{             
    vec3 colour = texture(full_screen, uvs).rgb;

    colour = apply_exposure(colour);
	colour = tone_map(colour);
    colour = apply_gamma(colour);

    fragment_colour = vec4(colour, 1.0);
}
