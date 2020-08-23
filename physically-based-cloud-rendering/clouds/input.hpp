#pragma once

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include "camera.hpp"

auto mouse_callback(GLFWwindow* /*window*/, double x_pos, double y_pos) -> void;
auto key_callback(GLFWwindow* window, int key, int /*scan_code*/, int action, int /*mode*/) -> void;
auto process_input(float dt, camera_t& camera) -> void;
[[nodiscard]] auto options() -> bool;
