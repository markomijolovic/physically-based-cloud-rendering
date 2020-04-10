#pragma once

#include <glm/detail/type_mat4x4.hpp>

#include "transform.hpp"

struct camera_t
{
	glm::mat4x4 projection{};
	transform_t transform{};
	float       movement_speed{5000.0F};
	float       mouse_sensitivity{0.1F};
};
