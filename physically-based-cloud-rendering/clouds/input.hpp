#pragma once

#define GLFW_INCLUDE_NONE

#include "camera.hpp"

#include <GLFW/glfw3.h>

auto               mouse_callback(GLFWwindow * /*window*/, double x_pos, double y_pos) noexcept -> void;
auto               key_callback(GLFWwindow *window, int key, int /*scan_code*/, int action, int /*mode*/) noexcept -> void;
auto               process_input(float dt, camera_t &camera) noexcept -> void;
[[nodiscard]] auto options() noexcept -> bool;
