#define GLFW_INCLUDE_NONE

#include <fstream>

#include <sstream>

#include "glbinding/gl/gl.h"

#include "glbinding/glbinding.h"

#include "GLFW/glfw3.h"

#include "stb_image.h"

constexpr auto screen_width  = 1280;
constexpr auto screen_height = 720;

constexpr float full_screen_quad[][3] =
{
	{-1.0F, -1.0F, 0.0F},
	{1.0F, -1.0F, 0.0F},
	{1.0F, 1.0F, 0.0F},
	{-1.0F, 1.0F, 0.0F}
};

constexpr int quad_indices[] = {0, 1, 2, 0, 2, 3};

int main()
{
	if (glfwInit() == 0)
	{
		std::exit(-1);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, 0);

	const auto window = glfwCreateWindow(screen_width, screen_height, "clouds", nullptr, nullptr);
	if (window == nullptr)
	{
		std::exit(-1);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(0); // disable v-sync
	glbinding::initialize(glfwGetProcAddress);
	gl::glViewport(0, 0, screen_width, screen_height);

	// load textures
	stbi_set_flip_vertically_on_load(1);
	gl::GLuint cloud_base_texture; // load base cloud shape texture

	{
		int        width;
		int        height;
		int        number_of_components;
		const auto cloud_base_image = stbi_load("textures/noise_shape.tga", &width, &height, &number_of_components, 0);

		gl::glGenTextures(1, &cloud_base_texture);
		glBindTexture(gl::GLenum::GL_TEXTURE_3D, cloud_base_texture);
		glTexParameteri(gl::GLenum::GL_TEXTURE_3D, gl::GLenum::GL_TEXTURE_MIN_FILTER, gl::GLenum::GL_LINEAR);
		glTexParameteri(gl::GLenum::GL_TEXTURE_3D, gl::GLenum::GL_TEXTURE_MAG_FILTER, gl::GLenum::GL_LINEAR);
		glTexImage3D(gl::GLenum::GL_TEXTURE_3D, 0, gl::GLenum::GL_RGBA8, 128, 128, 128, 0, gl::GLenum::GL_RGBA,
		             gl::GLenum::GL_UNSIGNED_BYTE, cloud_base_image);

		stbi_image_free(cloud_base_image);
	}

	gl::GLuint cloud_erosion_texture; // load cloud erosion texture
	{
		int        width;
		int        height;
		int        number_of_components;
		const auto cloud_erosion_image = stbi_load("textures/noise_erosion.tga", &width, &height, &number_of_components,
		                                           0);

		gl::glGenTextures(1, &cloud_erosion_texture);
		glBindTexture(gl::GLenum::GL_TEXTURE_3D, cloud_erosion_texture);
		glTexParameteri(gl::GLenum::GL_TEXTURE_3D, gl::GLenum::GL_TEXTURE_MIN_FILTER, gl::GLenum::GL_LINEAR);
		glTexParameteri(gl::GLenum::GL_TEXTURE_3D, gl::GLenum::GL_TEXTURE_MAG_FILTER, gl::GLenum::GL_LINEAR);
		glTexImage3D(gl::GLenum::GL_TEXTURE_3D, 0, gl::GLenum::GL_RGBA8, 32, 32, 32, 0, gl::GLenum::GL_RGBA,
		             gl::GLenum::GL_UNSIGNED_BYTE, cloud_erosion_image);

		stbi_image_free(cloud_erosion_image);
	}

	// create buffers for full screen quad
	gl::GLuint vbo;
	gl::GLuint vao;
	gl::GLuint ebo;
	gl::glGenBuffers(1, &vbo);
	gl::glGenBuffers(1, &ebo);
	gl::glGenVertexArrays(1, &vao);

	gl::glBindVertexArray(vao);
	glBindBuffer(gl::GLenum::GL_ARRAY_BUFFER, vbo);
	glBufferData(gl::GLenum::GL_ARRAY_BUFFER, 12 * sizeof(float), full_screen_quad, gl::GLenum::GL_STATIC_DRAW);
	glBindBuffer(gl::GLenum::GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(gl::GLenum::GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(int), quad_indices, gl::GLenum::GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, gl::GLenum::GL_FLOAT, false, 0, reinterpret_cast<void*>(0));
	gl::glEnableVertexAttribArray(0);

	// load cloud shader
	std::string vertex_code{};
	std::string fragment_code{};

	std::ifstream vertex_file{"cloud_vertex.glsl"};
	std::ifstream fragment_file{"cloud_fragment.glsl"};

	std::stringstream vertex_stream{};
	std::stringstream fragment_stream{};
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

	gl::glClearColor(0.0F, 0.0F, 0.0F, 1.0F);

	while (glfwWindowShouldClose(window) == 0)
	{
		glfwPollEvents();

		// do stuff
		glClear(gl::ClearBufferMask::GL_COLOR_BUFFER_BIT | gl::ClearBufferMask::GL_DEPTH_BUFFER_BIT);
		gl::glUseProgram(program);
		glDrawElements(gl::GLenum::GL_TRIANGLES, 6, gl::GLenum::GL_UNSIGNED_INT, nullptr);

		glfwSwapBuffers(window);
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}
