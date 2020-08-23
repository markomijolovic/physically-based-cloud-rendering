#pragma once

#include <glm/detail/type_mat4x4.hpp>

static inline constexpr float pi = 3.1415926535897932384626433832795029F;

[[nodiscard]] inline auto radians(float angle) { return angle * (pi / 180.0F); }

[[nodiscard]] auto translate(float x, float y, float z) -> glm::mat4x4;
[[nodiscard]] auto translate(const glm::vec3& vec3) -> glm::mat4x4;
[[nodiscard]] auto scale(float x, float y, float z) -> glm::mat4x4;
[[nodiscard]] auto scale(const glm::vec3& vec3) -> glm::mat4x4;
[[nodiscard]] auto scale(float x) -> glm::mat4x4;
[[nodiscard]] auto rotate(float angle, const glm::vec3& axis) -> glm::mat4x4;
[[nodiscard]] auto perspective(float fov_y, float aspect, float z_near, float z_far) -> glm::mat4x4;
