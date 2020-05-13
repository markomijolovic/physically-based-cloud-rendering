#version 460 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uvs;

out vec2 uvs;

void main()
{
	uvs = in_uvs;
	gl_Position = vec4(in_position, 1.0);
}
