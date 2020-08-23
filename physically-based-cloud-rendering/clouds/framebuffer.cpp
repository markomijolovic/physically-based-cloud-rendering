#include "framebuffer.hpp"

#include <glbinding/gl/functions.h>


framebuffer_t::framebuffer_t(std::uint32_t width,
                             std::uint32_t height,
                             std::uint32_t num_colour_attachments,
                             bool          depth,
                             gl::GLenum    sized_internal_format,
                             gl::GLenum    format,
                             gl::GLenum    type) : width{width},
                                                   height{height}
{
    gl::glGenFramebuffers(1, &id);
    bind();

    for (auto i{std::uint32_t{}}; i < num_colour_attachments; i++)
    {
        colour_attachments.emplace_back(width, height, 0, nullptr, sized_internal_format, format, type);
        glFramebufferTexture2D(gl::GLenum::GL_FRAMEBUFFER,
                               gl::GL_COLOR_ATTACHMENT0 + i,
                               gl::GLenum::GL_TEXTURE_2D,
                               colour_attachments.back().id,
                               0);
    }

    if (depth)
    {
        depth_and_stenctil_texture = std::make_unique<texture_t<2U>>(width,
                                                                     height,
                                                                     0,
                                                                     nullptr,
                                                                     gl::GLenum::GL_R32F,
                                                                     gl::GLenum::GL_DEPTH_COMPONENT,
                                                                     gl::GLenum::GL_FLOAT);

        glFramebufferTexture2D(gl::GLenum::GL_FRAMEBUFFER,
                               gl::GLenum::GL_DEPTH_ATTACHMENT,
                               gl::GLenum::GL_TEXTURE_2D,
                               depth_and_stenctil_texture->id,
                               0);
    }

    unbind();
}

framebuffer_t::~framebuffer_t() noexcept { gl::glDeleteFramebuffers(1, &id); }

auto framebuffer_t::bind() const -> void { glBindFramebuffer(gl::GL_FRAMEBUFFER, id); }

auto framebuffer_t::unbind() -> void { glBindFramebuffer(gl::GL_FRAMEBUFFER, 0); }
