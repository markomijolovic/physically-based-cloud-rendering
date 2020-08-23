#include "shader.hpp"

#include <fstream>
#include <sstream>
#include <string>

shader_t::~shader_t() { gl::glDeleteProgram(id_); }

shader_t::shader_t(const char* vertex_path, const char* fragment_path)
{
    auto vertex_code   = std::string{};
    auto fragment_code = std::string{};

    auto vertex_file{std::ifstream{vertex_path}};
    auto fragment_file{std::ifstream{fragment_path}};

    auto vertex_stream{std::stringstream{}};
    auto fragment_stream{std::stringstream{}};

    vertex_stream << vertex_file.rdbuf();
    fragment_stream << fragment_file.rdbuf();

    vertex_code   = vertex_stream.str();
    fragment_code = fragment_stream.str();

    auto p_vertex_data   = vertex_code.data();
    auto p_fragment_data = fragment_code.data();

    auto vertex = glCreateShader(gl::GLenum::GL_VERTEX_SHADER);
    gl::glShaderSource(vertex, 1, &p_vertex_data, nullptr);
    gl::glCompileShader(vertex);

    auto fragment = glCreateShader(gl::GLenum::GL_FRAGMENT_SHADER);
    gl::glShaderSource(fragment, 1, &p_fragment_data, nullptr);
    gl::glCompileShader(fragment);

    auto program = gl::glCreateProgram();
    gl::glAttachShader(program, vertex);
    gl::glAttachShader(program, fragment);
    gl::glLinkProgram(program);

    gl::glDeleteShader(vertex);
    gl::glDeleteShader(fragment);

    id_ = program;
}

auto shader_t::use() const -> void { gl::glUseProgram(id_); }
