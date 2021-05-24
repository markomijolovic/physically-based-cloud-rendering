#pragma once

#include <glm/vec3.hpp>

struct aabb_t {
    glm::vec3 min_coords{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()};
    glm::vec3 max_coords{
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max()};
};
