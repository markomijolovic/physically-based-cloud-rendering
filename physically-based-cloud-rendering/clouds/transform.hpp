#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

struct transform_t
{
    glm::vec4 position{0.0F, 0.0F, 0.0F, 1.0F};
    glm::vec3 rotation{};
    glm::vec3 scale{1.0F, 1.0F, 1.0F};

    [[nodiscard]] auto get_transform_matrix() const -> glm::mat4x4;
    [[nodiscard]] auto get_view_matrix() const -> glm::mat4x4;
};
