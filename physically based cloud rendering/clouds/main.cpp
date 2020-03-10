#include <glbinding/gl/gl.h>

#include <glbinding/glbinding.h>

#include <GLFW/glfw3.h>

using namespace gl;
using namespace glbinding;

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

	initialize(glfwGetProcAddress);

	gl::glViewport(0, 0, screen_width, screen_height);

	while (glfwWindowShouldClose(window) == 0)
	{
		glfwPollEvents();

		// do stuff

		glfwSwapBuffers(window);
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}
