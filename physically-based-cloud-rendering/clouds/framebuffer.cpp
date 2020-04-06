#include "framebuffer.hpp"

#include <glbinding/gl/functions.h>


framebuffer_t::framebuffer_t(unsigned width, unsigned height, unsigned num_colour_attachments, bool depth,
	gl::GLenum sized_internal_format, gl::GLenum format, gl::GLenum type)
	: width{ width },
	height{ height }
{
	gl::glGenFramebuffers(1, &id);
	bind();

	for (uint32_t i = 0; i < num_colour_attachments; i++)
	{
		colour_attachments.emplace_back(width, height, sized_internal_format, format, type);
		gl::glFramebufferTexture2D(gl::GLenum::GL_FRAMEBUFFER,
			gl::GL_COLOR_ATTACHMENT0 + i,
			gl::GLenum::GL_TEXTURE_2D,
			colour_attachments.back().id,
			0);
	}

	if (depth)
	{
		depth_and_stenctil_texture = std::make_unique<texture_t>(width, height, gl::GLenum::GL_DEPTH_COMPONENT32F, gl::GLenum::GL_DEPTH_COMPONENT, gl::GLenum::GL_FLOAT);

		gl::glFramebufferTexture2D(gl::GLenum::GL_FRAMEBUFFER, gl::GLenum::GL_DEPTH_ATTACHMENT, gl::GLenum::GL_TEXTURE_2D, depth_and_stenctil_texture->id, 0);
	}

	unbind();
}

framebuffer_t::~framebuffer_t()
{
	gl::glDeleteFramebuffers(1, &id);
}


void framebuffer_t::bind() const
{
	gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, id);
}

void framebuffer_t::unbind()
{
	gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, 0);
}
