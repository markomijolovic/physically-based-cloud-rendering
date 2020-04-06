#pragma once

#include <glm/vec3.hpp>

#include <glm/mat4x4.hpp>

struct transform_t
{
	glm::vec4 position{0.0F, 0.0F, 0.0F, 1.0F};
	glm::vec3 rotation{};
	glm::vec3 scale{ 1.0F, 1.0F, 1.0F };

	[[nodiscard]] glm::mat4x4  get_transform_matrix() const;
	[[nodiscard]] glm::mat4x4  get_view_matrix() const;
};
