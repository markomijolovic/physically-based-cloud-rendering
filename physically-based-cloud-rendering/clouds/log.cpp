#include "log.hpp"

#include <glbinding/gl/functions.h>

#include <iostream>

void log(const std::string &string)
{
	std::clog << string << '\n';
}

void log_opengl_error()
{
	auto error = static_cast<unsigned int>(gl::glGetError());
	if (error)
	{
		std::clog << "OpenGL error " << error << '\n';
	}
}
