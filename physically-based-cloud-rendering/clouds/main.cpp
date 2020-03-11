#define GLFW_INCLUDE_NONE

#define STB_IMAGE_IMPLEMENTATION

#include "glbinding/gl/gl.h"

#include "glbinding/glbinding.h"

#include "GLFW/glfw3.h"

#include "stb_image.h"

constexpr auto screen_width = 1280;
constexpr auto screen_height = 720;

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

	int width;
	int height;
	int number_of_components;
	const auto cloud_base_image = stbi_load("textures/noise_shape.tga", &width, &height, &number_of_components, 0);

	gl::GLuint cloud_base_texture;
	gl::glGenTextures(1, &cloud_base_texture);
	gl::glBindTexture(gl::GLenum::GL_TEXTURE_3D, cloud_base_texture);
	gl::glTexParameteri(gl::GLenum::GL_TEXTURE_3D, gl::GLenum::GL_TEXTURE_MIN_FILTER, gl::GLenum::GL_LINEAR);
	gl::glTexParameteri(gl::GLenum::GL_TEXTURE_3D, gl::GLenum::GL_TEXTURE_MAG_FILTER, gl::GLenum::GL_LINEAR);
	gl::glTexImage3D(gl::GLenum::GL_TEXTURE_3D, 0, gl::GLenum::GL_RGBA8, 128, 128, 128, 0, gl::GLenum::GL_RGBA,
	             gl::GLenum::GL_UNSIGNED_BYTE, cloud_base_image);

	const auto cloud_erosion_image = stbi_load("textures/noise_erosion.tga", &width, &height, &number_of_components, 0);

	gl::GLuint cloud_erosion_texture;
	gl::glGenTextures(1, &cloud_erosion_texture);
	gl::glBindTexture(gl::GLenum::GL_TEXTURE_3D, cloud_erosion_texture);
	gl::glTexParameteri(gl::GLenum::GL_TEXTURE_3D, gl::GLenum::GL_TEXTURE_MIN_FILTER, gl::GLenum::GL_LINEAR);
	gl::glTexParameteri(gl::GLenum::GL_TEXTURE_3D, gl::GLenum::GL_TEXTURE_MAG_FILTER, gl::GLenum::GL_LINEAR);
	gl::glTexImage3D(gl::GLenum::GL_TEXTURE_3D, 0, gl::GLenum::GL_RGBA8, 32, 32, 32, 0, gl::GLenum::GL_RGBA,
	             gl::GLenum::GL_UNSIGNED_BYTE, cloud_erosion_image);

	while (glfwWindowShouldClose(window) == 0)
	{
		glfwPollEvents();

		// do stuff

		glfwSwapBuffers(window);
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}
