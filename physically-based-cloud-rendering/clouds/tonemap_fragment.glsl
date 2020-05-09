#version 460 core
out vec4 fragment_colour;
  
in vec2 uvs;
uniform sampler2D full_screen;

vec3 tone_map(vec3 x) 
{
    return x/(x+1);
}

void main()
{             
    vec3 colour = texture(full_screen, uvs).rgb;

    colour = tone_map(colour);

    colour.r = pow(colour.r, 1.0/2.2);
    colour.g = pow(colour.g, 1.0/2.2);
    colour.b = pow(colour.b, 1.0/2.2);

    fragment_colour = vec4(colour, 1.0);
}
