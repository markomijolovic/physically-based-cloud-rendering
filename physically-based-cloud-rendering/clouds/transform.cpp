#include "transform.hpp"

#include "transforms.hpp"

auto get_transform_matrix(const transform_t &transform) -> glm::mat4x4
{
    auto transform_matrix = translate(transform.position);
    transform_matrix *= rotate(radians(transform.rotation.z), {0.0F, 0.0F, 1.0F});
    transform_matrix *= rotate(radians(transform.rotation.y), {0.0F, 1.0F, 0.0F});
    transform_matrix *= rotate(radians(transform.rotation.x), {1.0F, 0.0F, 0.0F});
    transform_matrix *= scale(transform.scale);

    return transform_matrix;
}

auto get_view_matrix(const transform_t &transform) -> glm::mat4x4
{
    auto transform_matrix = rotate(radians(-transform.rotation.x), {1.0F, 0.0F, 0.0F});
    transform_matrix      = transform_matrix * rotate(radians(-transform.rotation.y), {0.0F, 1.0F, 0.0F});

    return transform_matrix * translate(-transform.position);
}
