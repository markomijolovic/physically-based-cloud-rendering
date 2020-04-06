#pragma once

#include <glbinding/gl/functions.h>

#include <glm/detail/type_mat4x4.hpp>

#include <string>

template <typename T>
void set_uniform(gl::GLuint id, const std::string& name, T&& value)
{
	using type = std::remove_cvref_t<T>;
	if constexpr (std::is_same_v<type, glm::mat4x4>)
	{
		gl::glUniformMatrix4fv(gl::glGetUniformLocation(id, name.c_str()),
		                       1,
		                       false,
		                       const_cast<gl::GLfloat*>(reinterpret_cast<const gl::GLfloat*>(&value[0])));
	}
	else if constexpr (std::is_same_v<type, glm::vec3>)
	{
		gl::glUniform3fv(gl::glGetUniformLocation(id, name.c_str()),
		                 1,
		                 const_cast<gl::GLfloat*>(reinterpret_cast<const gl::GLfloat*>(&value)));
	}
	else if constexpr (std::is_same_v<type, glm::vec4>)
	{
		gl::glUniform4fv(gl::glGetUniformLocation(id, name.c_str()),
		                 1,
		                 const_cast<gl::GLfloat*>(reinterpret_cast<const gl::GLfloat*>(&value)));
	}
	else if constexpr (std::is_same_v<type, bool>)
	{
		gl::glUniform1i(gl::glGetUniformLocation(id, name.c_str()), static_cast<gl::GLint>(value));
	}
	else if constexpr (std::is_same_v<type, int32_t>)
	{
		gl::glUniform1i(gl::glGetUniformLocation(id, name.c_str()), value);
	}
	else if constexpr (std::is_same_v<type, uint32_t>)
	{
		gl::glUniform1i(gl::glGetUniformLocation(id, name.c_str()), static_cast<int>(value));
	}
	else if constexpr (std::is_same_v<type, float>)
	{
		gl::glUniform1f(gl::glGetUniformLocation(id, name.c_str()), value);
	}
}
