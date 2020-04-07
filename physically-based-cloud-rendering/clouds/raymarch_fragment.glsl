#version 460 core

out vec4 fragment_colour;

in vec2 uvs;

uniform sampler2D full_screen;

void main()
{
	fragment_colour = texture(full_screen, uvs);
}
