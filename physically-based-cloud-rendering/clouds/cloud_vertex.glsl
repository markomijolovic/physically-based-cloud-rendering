#version 460 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uvs;

out vec2 uvs;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
	gl_Position = projection*view*model*vec4(in_position, 1.0);
}
