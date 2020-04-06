#pragma once

#include <cstdint>

#include <glbinding/gl/enum.h>

#include <memory>

#include <vector>

#include "texture.hpp"

struct framebuffer_t
{
	framebuffer_t(uint32_t width, uint32_t height, uint32_t num_colour_attachments,
	              bool         depth,
	              gl::GLenum   sized_internal_format = gl::GLenum::GL_RGBA8, gl::GLenum format = gl::GLenum::GL_RGBA, gl::GLenum type = gl::GLenum::GL_UNSIGNED_BYTE);
	~framebuffer_t();

	void bind() const;
	static void unbind();

	uint32_t id{};
	uint32_t width{};
	uint32_t height{};
	std::unique_ptr<texture_t> depth_and_stenctil_texture{};
	std::vector<texture_t> colour_attachments{};
};
