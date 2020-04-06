#include "texture.hpp"

#include <glbinding/gl/functions.h>

texture_t::texture_t(uint32_t width, uint32_t height, gl::GLenum sized_internal_format, gl::GLenum format,
                     gl::GLenum type, gl::GLenum filtering_mag, gl::GLenum filtering_min, gl::GLenum wrap_s, gl::GLenum wrap_t)
	:
	width(width),
	height(height),
	sized_internal_format(sized_internal_format),
	format(format),
	type(type),
	filtering_mag(filtering_mag),
	filtering_min(filtering_min),
	wrap_s(wrap_s),
	wrap_t(wrap_t)
{
	gl::glGenTextures(1, &id);

	bind();

	gl::glTexStorage2D(gl::GLenum::GL_TEXTURE_2D, 1, sized_internal_format, width, height);
	gl::glTexParameteri(gl::GLenum::GL_TEXTURE_2D, gl::GLenum::GL_TEXTURE_MIN_FILTER, filtering_min);
	gl::glTexParameteri(gl::GLenum::GL_TEXTURE_2D, gl::GLenum::GL_TEXTURE_MAG_FILTER, filtering_mag);
	gl::glTexParameteri(gl::GLenum::GL_TEXTURE_2D, gl::GLenum::GL_TEXTURE_WRAP_S, wrap_s);

	gl::glTexParameteri(gl::GLenum::GL_TEXTURE_2D, gl::GLenum::GL_TEXTURE_WRAP_T, wrap_t);

	unbind();
}

texture_t::~texture_t()
{
	gl::glDeleteTextures(1, &id);
}

void texture_t::unbind()
{
	gl::glBindTexture(gl::GLenum::GL_TEXTURE_2D, 0);
}

void texture_t::bind(int32_t unit) const
{
	if (unit >= 0)
	{
		gl::glActiveTexture(gl::GLenum::GL_TEXTURE0 + unit);
	}
	gl::glBindTexture(gl::GLenum::GL_TEXTURE_2D, id);
}
