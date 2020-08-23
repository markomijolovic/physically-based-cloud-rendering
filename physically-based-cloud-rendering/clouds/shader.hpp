#pragma once

#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>

class shader_t
{
public:
    shader_t() = default;
    shader_t(const char* vertex_path, const char* fragment_path);
    ~shader_t();

    auto use() const -> void;

    template <typename T>
    auto set_uniform(const char* name, T&& value) const -> void
    {
        using type = std::remove_cvref_t<T>;
        if constexpr (std::is_same_v<type, glm::mat4x4>)
        {
            gl::glUniformMatrix4fv(gl::glGetUniformLocation(id_, name), 1, false, const_cast<float*>(reinterpret_cast<
                const float*>(&value[0]))
            );
        }
        else if constexpr (std::is_same_v<type, glm::vec3>)
        {
            gl::glUniform3fv(gl::glGetUniformLocation(id_, name),
                             1,
                             const_cast<float*>(reinterpret_cast<const float*>(&value)));
        }
        else if constexpr (std::is_same_v<type, glm::vec2>)
        {
            gl::glUniform2fv(gl::glGetUniformLocation(id_, name),
                             1,
                             const_cast<float*>(reinterpret_cast<const float*>(&value)));
        }
        else if constexpr (std::is_same_v<type, glm::vec4>)
        {
            gl::glUniform4fv(gl::glGetUniformLocation(id_, name),
                             1,
                             const_cast<float*>(reinterpret_cast<const float*>(&value)));
        }
        else if constexpr (std::is_same_v<type, bool>)
        {
            gl::glUniform1i(gl::glGetUniformLocation(id_, name), static_cast<std::int32_t>(value));
        }
        else if constexpr (std::is_same_v<type, std::int32_t>)
        {
            gl::glUniform1i(gl::glGetUniformLocation(id_, name), value);
        }
        else if constexpr (std::is_same_v<type, std::uint32_t>)
        {
            gl::glUniform1i(gl::glGetUniformLocation(id_, name), static_cast<std::int32_t>(value));
        }
        else if constexpr (std::is_same_v<type, float>) { gl::glUniform1f(gl::glGetUniformLocation(id_, name), value); }
    }

private:
    std::uint32_t id_{};
};
