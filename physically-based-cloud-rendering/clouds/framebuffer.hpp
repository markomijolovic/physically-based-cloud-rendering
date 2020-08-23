#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <glbinding/gl/enum.h>

#include "texture.hpp"

struct framebuffer_t
{
    framebuffer_t(std::uint32_t width,
                  std::uint32_t height,
                  std::uint32_t num_colour_attachments,
                  bool          depth,
                  gl::GLenum    sized_internal_format = gl::GLenum::GL_RGBA16F,
                  gl::GLenum    format                = gl::GLenum::GL_RGBA,
                  gl::GLenum    type                  = gl::GLenum::GL_FLOAT);
    framebuffer_t(const framebuffer_t& rhs)                        = delete;
    framebuffer_t(framebuffer_t&& rhs) noexcept                    = delete;
    auto operator=(const framebuffer_t& rhs) -> framebuffer_t&     = delete;
    auto operator=(framebuffer_t&& rhs) noexcept -> framebuffer_t& = delete;
    ~framebuffer_t() noexcept;

    auto        bind() const -> void;
    static auto unbind() -> void;

    std::uint32_t                  id{};
    std::uint32_t                  width{};
    std::uint32_t                  height{};
    std::unique_ptr<texture_t<2U>> depth_and_stenctil_texture{};
    std::vector<texture_t<2U>>     colour_attachments{};
};
