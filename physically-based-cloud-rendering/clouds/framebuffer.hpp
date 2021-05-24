#pragma once

#include "texture.hpp"

#include <cstdint>
#include <glbinding/gl/enum.h>
#include <memory>
#include <vector>

class framebuffer_t {
public:
    framebuffer_t(std::uint32_t width,
                  std::uint32_t height,
                  std::uint32_t num_colour_attachments,
                  bool          depth,
                  gl::GLenum    sized_internal_format = gl::GLenum::GL_RGBA16F,
                  gl::GLenum    format                = gl::GLenum::GL_RGBA,
                  gl::GLenum    type                  = gl::GLenum::GL_FLOAT) noexcept;
    framebuffer_t(const framebuffer_t &rhs) = delete;
    framebuffer_t(framebuffer_t &&rhs)      = delete; // YAGNI
    auto operator=(const framebuffer_t &rhs) -> framebuffer_t & = delete;
    auto operator=(framebuffer_t &&rhs) -> framebuffer_t & = delete; // YAGNI
    ~framebuffer_t() noexcept;

    auto        bind() const noexcept -> void;
    static auto unbind() noexcept -> void;

    [[nodiscard]] auto depth_and_stencil() const noexcept -> const texture_t<2U> &;
    [[nodiscard]] auto depth_and_stencil() noexcept -> texture_t<2U> &;
    [[nodiscard]] auto colour_attachments() const noexcept -> const std::vector<texture_t<2U>> &;
    [[nodiscard]] auto colour_attachments() noexcept -> std::vector<texture_t<2U>> &;

private:
    std::uint32_t                  id_{};
    std::uint32_t                  width_{};
    std::uint32_t                  height_{};
    std::unique_ptr<texture_t<2U>> depth_stencil_{};
    std::vector<texture_t<2U>>     colour_attachments_{};
};
