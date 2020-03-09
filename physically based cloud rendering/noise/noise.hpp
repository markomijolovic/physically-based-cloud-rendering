#pragma once

#include "glm/gtc/noise.hpp"

// returns a tile-able worley noise value in range [0, 1]
// point is a 3d point in range [0, 1]
float worley(const glm::vec3& point, float cell_count);

// returns a tile-able perlin noise value in [0, 1]
// p is a 3d point in range [0, 1]
float perlin(const glm::vec3& p, float frequency, int octave_count);
