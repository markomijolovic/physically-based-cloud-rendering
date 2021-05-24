#pragma once

#include <glm/glm.hpp>
#include <vector>

class mesh_t {
public:
    mesh_t(std::vector<glm::vec3> positions, std::vector<glm::vec2> uvs, std::vector<std::uint32_t> indices);
    mesh_t(const mesh_t &rhs) = delete;
    mesh_t(mesh_t &&rhs)      = delete; // YAGNI
    auto operator=(const mesh_t &rhs) -> mesh_t & = delete;
    auto operator=(mesh_t &&rhs) -> mesh_t & = delete; // YAGNI
    ~mesh_t() noexcept;

    auto draw() const -> void;

private:
    std::vector<glm::vec3>     positions_{};
    std::vector<glm::vec2>     uvs_{};
    std::vector<std::uint32_t> indices_{};
    std::uint32_t              vao_{};
    std::uint32_t              vbo_{};
    std::uint32_t              ebo_{};
};
