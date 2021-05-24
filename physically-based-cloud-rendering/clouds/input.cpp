#include "input.hpp"

#include "transforms.hpp"

#include <algorithm>
#include <unordered_map>

namespace {
auto buttons{std::unordered_map<int, bool>{}};
auto first_mouse{true};
auto show_options{false};
auto x_offset{0.0F};
auto y_offset{0.0F};
auto x{0.0F};
auto y{0.0F};
} // namespace

auto process_input(float dt, camera_t &camera) noexcept -> void
{
    if (show_options) {
        return;
    }

    auto view_matrix = get_view_matrix(camera.transform);

    const auto right   = glm::vec3{view_matrix[0][0], view_matrix[1][0], view_matrix[2][0]};
    const auto up      = glm::vec3{view_matrix[0][1], view_matrix[1][1], view_matrix[2][1]};
    const auto forward = -glm::vec3{view_matrix[0][2], view_matrix[1][2], view_matrix[2][2]};

    glm::vec3 trans{};

    if (buttons[GLFW_KEY_W]) {
        trans += forward * dt * camera.movement_speed;
    }

    if (buttons[GLFW_KEY_S]) {
        trans -= forward * dt * camera.movement_speed;
    }

    if (buttons[GLFW_KEY_D]) {
        trans += right * dt * camera.movement_speed;
    }

    if (buttons[GLFW_KEY_A]) {
        trans -= right * dt * camera.movement_speed;
    }

    if (buttons[GLFW_KEY_Q]) {
        trans += up * dt * camera.movement_speed;
    }

    if (buttons[GLFW_KEY_E]) {
        trans -= up * dt * camera.movement_speed;
    }

    camera.transform.position = translate(trans) * camera.transform.position;

    if (x_offset != 0.0F) {
        camera.transform.rotation.y -= x_offset * camera.mouse_sensitivity;
        camera.transform.rotation.y = std::fmod(camera.transform.rotation.y, 360.0F);
        x_offset                    = 0.0F;
    }

    if (y_offset != 0.0F) {
        camera.transform.rotation.x -= y_offset * camera.mouse_sensitivity;
        camera.transform.rotation.x = std::clamp(camera.transform.rotation.x, -89.0F, 89.0F);
        y_offset                    = 0.0F;
    }
}

auto key_callback(GLFWwindow *window, int key, int /*scan_code*/, int action, int /*mode*/) noexcept -> void
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, 1);
        return;
    }

    if (key == GLFW_KEY_O && action == GLFW_PRESS) {
        if (!show_options) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        show_options = !show_options;
    } else if (action == GLFW_PRESS) {
        buttons[key] = true;
    } else if (action == GLFW_RELEASE) {
        buttons[key] = false;
    }
}

auto mouse_callback(GLFWwindow * /*window*/, double x_pos, double y_pos) noexcept -> void
{
    if (first_mouse) {
        x           = static_cast<float>(x_pos);
        y           = static_cast<float>(y_pos);
        first_mouse = false;
    }

    x_offset = static_cast<float>(x_pos) - x;
    y_offset = static_cast<float>(y_pos) - y;

    x = static_cast<float>(x_pos);
    y = static_cast<float>(y_pos);
}

auto options() noexcept -> bool { return show_options; }
