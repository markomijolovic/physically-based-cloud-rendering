#pragma once

#include "transform.hpp"

#include <glm/detail/type_mat4x4.hpp>

struct camera_t {
    glm::mat4x4 projection{};
    transform_t transform{};
    float       movement_speed{2.5F};
    float       mouse_sensitivity{0.1F};
};
