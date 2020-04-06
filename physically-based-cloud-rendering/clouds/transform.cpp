#include "transform.hpp"

#include "transforms.hpp"

glm::mat4x4 transform_t::get_transform_matrix() const
{
	auto transform_matrix = translate(position);
	transform_matrix *= rotate(radians(rotation.z), {0.0F, 0.0F, 1.0F});
	transform_matrix *= rotate(radians(rotation.y), {0.0F, 1.0F, 0.0F});
	transform_matrix *= rotate(radians(rotation.x), {1.0F, 0.0F, 0.0F});
	transform_matrix *= ::scale(scale);

	return transform_matrix;
}

glm::mat4x4 transform_t::get_view_matrix() const
{
	auto transform_matrix = rotate(radians(-rotation.x), {1.0F, 0.0F, 0.0F});
	transform_matrix      = transform_matrix * rotate(radians(-rotation.y), {0.0F, 1.0F, 0.0F});

	return transform_matrix * translate(-position);
}
