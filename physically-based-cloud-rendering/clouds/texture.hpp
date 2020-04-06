#pragma once

#include <cstdint>

#include <glbinding/gl/enum.h>

struct texture_t
{
	texture_t(uint32_t width, uint32_t height, gl::GLenum sized_internal_format = gl::GLenum::GL_RGBA8, gl::GLenum format = gl::GLenum::GL_RGBA,
		gl::GLenum type = gl::GLenum::GL_UNSIGNED_BYTE, gl::GLenum filtering_mag = gl::GLenum::GL_LINEAR, gl::GLenum filtering_min = gl::GLenum::GL_LINEAR, gl::GLenum wrap_s = gl::GLenum::GL_REPEAT, gl::GLenum wrap_t = gl::GLenum::GL_REPEAT);
	~texture_t();
	static void unbind();
	void bind(int32_t unit = -1) const;


	uint32_t  id{};
	uint32_t width{};
	uint32_t height{};
	gl::GLenum sized_internal_format = gl::GLenum::GL_RGBA8;
	gl::GLenum format = gl::GLenum::GL_RGBA;
	gl::GLenum type = gl::GLenum::GL_UNSIGNED_BYTE;
	gl::GLenum filtering_mag = gl::GLenum::GL_LINEAR;
	gl::GLenum filtering_min = gl::GLenum::GL_LINEAR;
	gl::GLenum wrap_s = gl::GLenum::GL_REPEAT;
	gl::GLenum wrap_t = gl::GLenum::GL_REPEAT;
};
