#include "framebuffer.hpp"

#include <cassert>
#include <glbinding/gl/functions.h>

framebuffer_t::framebuffer_t(std::uint32_t width,
                             std::uint32_t height,
                             std::uint32_t num_colour_attachments,
                             bool          depth,
                             gl::GLenum    sized_internal_format,
                             gl::GLenum    format,
                             gl::GLenum    type) noexcept
    : width_{width}
    , height_{height}
{
    gl::glGenFramebuffers(1, &id_);

    bind();
    for (auto i{std::uint32_t{}}; i < num_colour_attachments; i++) {
        colour_attachments_.emplace_back(width, height, 0, nullptr, sized_internal_format, format, type);
        glFramebufferTexture2D(gl::GLenum::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0 + i, gl::GLenum::GL_TEXTURE_2D, colour_attachments_.back().id(), 0);
    }

    if (depth) {
        depth_stencil_ = std::make_unique<texture_t<2U>>(width, height, 0, nullptr, gl::GLenum::GL_R32F, gl::GLenum::GL_DEPTH_COMPONENT, gl::GLenum::GL_FLOAT);

        glFramebufferTexture2D(gl::GLenum::GL_FRAMEBUFFER, gl::GLenum::GL_DEPTH_ATTACHMENT, gl::GLenum::GL_TEXTURE_2D, depth_stencil_->id(), 0);
    }

    unbind();
}

framebuffer_t::~framebuffer_t() noexcept
{
    gl::glDeleteFramebuffers(1, &id_);
}

auto framebuffer_t::bind() const noexcept -> void
{
    assert(id_);

    glBindFramebuffer(gl::GL_FRAMEBUFFER, id_);
}

auto framebuffer_t::unbind() noexcept -> void { glBindFramebuffer(gl::GL_FRAMEBUFFER, 0); }

auto framebuffer_t::depth_and_stencil() const noexcept -> const texture_t<2> &
{
    assert(depth_stencil_);

    return *depth_stencil_;
}

auto framebuffer_t::depth_and_stencil() noexcept -> texture_t<2> &
{
    assert(depth_stencil_);

    return *depth_stencil_;
}

auto framebuffer_t::colour_attachments() const noexcept -> const std::vector<texture_t<2>> &
{
    return colour_attachments_;
}

auto framebuffer_t::colour_attachments() noexcept -> std::vector<texture_t<2>> &
{
    return colour_attachments_;
}
