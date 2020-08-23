#include "mesh.hpp"

#include <utility>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>


mesh_t::mesh_t(std::vector<glm::vec3> positions, std::vector<glm::vec2> uvs, std::vector<std::uint32_t> indices) :
    positions_{std::move(positions)},
    uvs_{std::move(uvs)},
    indices_{std::move(indices)}
{
    gl::glGenVertexArrays(1, &vao_);
    gl::glGenBuffers(1, &vbo_);
    gl::glGenBuffers(1, &ebo_);

    std::vector<float> data{};
    for (size_t i = 0; i < positions_.size(); i++)
    {
        data.push_back(positions_[i].x);
        data.push_back(positions_[i].y);
        data.push_back(positions_[i].z);

        data.push_back(uvs_[i].x);
        data.push_back(uvs_[i].y);
    }

    gl::glBindVertexArray(vao_);
    glBindBuffer(gl::GLenum::GL_ARRAY_BUFFER, vbo_);
    glBufferData(gl::GLenum::GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), gl::GLenum::GL_STATIC_DRAW);

    glBindBuffer(gl::GLenum::GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(gl::GLenum::GL_ELEMENT_ARRAY_BUFFER,
                 indices_.size() * sizeof(std::uint32_t),
                 indices_.data(),
                 gl::GLenum::GL_STATIC_DRAW);

    constexpr auto stride = 5 * sizeof(float);
    size_t         offset{};

    glVertexAttribPointer(0,
                          3,
                          gl::GLenum::GL_FLOAT,
                          0U,
                          static_cast<gl::GLsizei>(stride),
                          reinterpret_cast<gl::GLvoid*>(offset));
    gl::glEnableVertexAttribArray(0);
    offset += 3 * sizeof(float);

    glVertexAttribPointer(1,
                          2,
                          gl::GLenum::GL_FLOAT,
                          false,
                          static_cast<gl::GLsizei>(stride),
                          reinterpret_cast<gl::GLvoid*>(offset));
    gl::glEnableVertexAttribArray(1);

    gl::glBindVertexArray(0);
}

auto mesh_t::draw() const -> void
{
    gl::glBindVertexArray(vao_);
    glDrawElements(gl::GLenum::GL_TRIANGLES,
                   static_cast<gl::GLsizei>(indices_.size()),
                   gl::GLenum::GL_UNSIGNED_INT,
                   nullptr);
    gl::glBindVertexArray(0);
}

mesh_t::~mesh_t() noexcept
{
    gl::glDeleteBuffers(1, &ebo_);
    gl::glDeleteBuffers(1, &vbo_);
    gl::glDeleteVertexArrays(1, &vao_);
}
