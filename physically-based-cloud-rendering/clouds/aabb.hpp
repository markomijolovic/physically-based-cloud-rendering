#pragma once

#include <glm/vec3.hpp>

struct aabb_t
{
    glm::vec3 min_coords{};
    glm::vec3 max_coords{};
};
