#pragma once

#include "glm/gtc/noise.hpp"

float worley(const glm::vec3& point, float cell_count);
float perlin(const glm::vec3& p, float frequency, int octave_count);
