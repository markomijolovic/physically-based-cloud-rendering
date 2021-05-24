#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <glbinding/gl/gl.h>
#include <iostream>
#include <stb_image.h>

template <std::uint32_t Dimensions>
class texture_t {
    static_assert(Dimensions == 1 || Dimensions == 2 || Dimensions == 3);

public:
    texture_t() = default;

    texture_t(std::uint32_t                  texture_width,
              [[maybe_unused]] std::uint32_t texture_height,
              [[maybe_unused]] std::uint32_t texture_depth,
              const char *                   texture_path          = nullptr,
              gl::GLenum                     sized_internal_format = gl::GLenum::GL_RGBA8,
              [[maybe_unused]] gl::GLenum    format                = gl::GLenum::GL_RGBA,
              [[maybe_unused]] gl::GLenum    type                  = gl::GLenum::GL_UNSIGNED_BYTE,
              gl::GLenum                     filtering_mag         = gl::GLenum::GL_LINEAR,
              gl::GLenum                     filtering_min         = gl::GLenum::GL_LINEAR,
              gl::GLenum                     wrap_s                = gl::GLenum::GL_REPEAT,
              gl::GLenum                     wrap_t                = gl::GLenum::GL_REPEAT) noexcept
    {
        gl::glGenTextures(1, &id_);
        assert(id_);

        if constexpr (Dimensions == 1) {
            glBindTexture(gl::GLenum::GL_TEXTURE_1D, id_);
            glTexParameteri(gl::GLenum::GL_TEXTURE_1D, gl::GLenum::GL_TEXTURE_MIN_FILTER, filtering_min);
            glTexParameteri(gl::GLenum::GL_TEXTURE_1D, gl::GLenum::GL_TEXTURE_MAG_FILTER, filtering_mag);
            glTexParameteri(gl::GLenum::GL_TEXTURE_1D, gl::GLenum::GL_TEXTURE_WRAP_S, wrap_s);
            glTexParameteri(gl::GLenum::GL_TEXTURE_1D, gl::GLenum::GL_TEXTURE_WRAP_T, wrap_t);

            const auto levels = static_cast<uint32_t>(floor(log2(texture_width))) + 1;
            glTexStorage1D(gl::GLenum::GL_TEXTURE_1D, levels, sized_internal_format, texture_width);

            if (texture_path != nullptr) {
                auto       width{0};
                auto       height{0};
                auto       number_of_components{0};
                const auto out = stbi_loadf(texture_path, &width, &height, &number_of_components, 0);

                glTexSubImage1D(gl::GLenum::GL_TEXTURE_1D, 0, 0, texture_width, format, type, out);
                stbi_image_free(out);
            }
        } else if constexpr (Dimensions == 2) {
            glBindTexture(gl::GLenum::GL_TEXTURE_2D, id_);
            glTexParameteri(gl::GLenum::GL_TEXTURE_2D, gl::GLenum::GL_TEXTURE_MIN_FILTER, filtering_min);
            glTexParameteri(gl::GLenum::GL_TEXTURE_2D, gl::GLenum::GL_TEXTURE_MAG_FILTER, filtering_mag);
            glTexParameteri(gl::GLenum::GL_TEXTURE_2D, gl::GLenum::GL_TEXTURE_WRAP_S, wrap_s);
            glTexParameteri(gl::GLenum::GL_TEXTURE_2D, gl::GLenum::GL_TEXTURE_WRAP_T, wrap_t);

            const auto levels = static_cast<uint32_t>(floor(log2(std::max(texture_width, texture_height)))) + 1;
            glTexStorage2D(gl::GLenum::GL_TEXTURE_2D, levels, sized_internal_format, texture_width, texture_height);

            if (texture_path != nullptr) {
                auto       width{0};
                auto       height{0};
                auto       number_of_components{0};
                const auto out = stbi_load(texture_path, &width, &height, &number_of_components, 0);
                glTexSubImage2D(gl::GLenum::GL_TEXTURE_2D, 0, 0, 0, texture_width, texture_height, format, type, out);
                glTexImage2D(gl::GLenum::GL_TEXTURE_2D, 0, sized_internal_format, texture_width, texture_height, 0, format, type, out);
                stbi_image_free(out);
            }
        } else {
            glBindTexture(gl::GLenum::GL_TEXTURE_3D, id_);
            //auto aniso = 0.0F;
            //glGetFloatv(gl::GLenum::GL_MAX_TEXTURE_MAX_ANISOTROPY, &aniso);
            //glTexParameterf(gl::GLenum::GL_TEXTURE_3D, gl::GLenum::GL_TEXTURE_MAX_ANISOTROPY, aniso);
            glTexParameteri(gl::GLenum::GL_TEXTURE_3D, gl::GLenum::GL_TEXTURE_MIN_FILTER, filtering_min);
            glTexParameteri(gl::GLenum::GL_TEXTURE_3D, gl::GLenum::GL_TEXTURE_MAG_FILTER, filtering_mag);

            const auto levels = static_cast<uint32_t>(floor(log2(std::max({texture_width, texture_height, texture_depth})))) + 1;
            glTexStorage3D(gl::GLenum::GL_TEXTURE_3D, levels, sized_internal_format, texture_width, texture_height, texture_depth);

            if (texture_path != nullptr) {
                auto       width{0};
                auto       height{0};
                auto       number_of_components{0};
                const auto out = stbi_load(texture_path, &width, &height, &number_of_components, 0);

                glTexSubImage3D(gl::GLenum::GL_TEXTURE_3D, 0, 0, 0, 0, texture_width, texture_height, texture_depth, format, type, out);
                stbi_image_free(out);
            }
        }
    }

    texture_t(const texture_t &rhs) = delete;

    texture_t(texture_t &&rhs) noexcept
        : id_{rhs.id_}
    {
        rhs.id_ = 0;
    }

    auto operator=(const texture_t &rhs) = delete;

    auto operator=(texture_t &&rhs) noexcept -> texture_t &
    {
        auto temp = std::move(rhs);
        swap(temp, *this);
        return *this;
    }

    ~texture_t() noexcept
    {
        gl::glDeleteTextures(1, &id_);
    }

    static auto unbind() noexcept -> void
    {
        if constexpr (Dimensions == 1) {
            glBindTexture(gl::GLenum::GL_TEXTURE_1D, 0);
        } else if constexpr (Dimensions == 2) {
            glBindTexture(gl::GLenum::GL_TEXTURE_2D, 0);
        } else {
            glBindTexture(gl::GLenum::GL_TEXTURE_3D, 0);
        }
    }

    auto bind(std::int32_t unit = -1) const noexcept -> void
    {
        assert(id_);

        if (unit >= 0) {
            glActiveTexture(gl::GLenum::GL_TEXTURE0 + unit);
        }

        if constexpr (Dimensions == 1) {
            glBindTexture(gl::GLenum::GL_TEXTURE_1D, id_);
        } else if constexpr (Dimensions == 2) {
            glBindTexture(gl::GLenum::GL_TEXTURE_2D, id_);
        } else {
            glBindTexture(gl::GLenum::GL_TEXTURE_3D, id_);
        }
    }

    [[nodiscard]] auto id() const noexcept -> std::uint32_t
    {
        return id_;
    }

    friend auto swap(texture_t &lhs, texture_t &rhs) noexcept -> void
    {
        std::swap(lhs.id_, rhs.id_);
    }

private:
    std::uint32_t id_{};
};
