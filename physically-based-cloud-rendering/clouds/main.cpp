#define GLFW_INCLUDE_NONE

#include <fstream>

#include <sstream>

#include <chrono>

#include "camera.hpp"

#include "glbinding/gl/gl.h"

#include "glbinding/glbinding.h"

#include "GLFW/glfw3.h"

#include "log.hpp"

#include "stb_image.h"

#include "transforms.hpp"

#include "uniforms.hpp"

#include "aabb.hpp"

#include "framebuffer.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

constexpr auto screen_width  = 1280;
constexpr auto screen_height = 720;

constexpr float full_screen_quad[][5] =
{
	{-1.0F, -1.0F, 0.0F, 0.0F, 0.0F},
	{1.0F, -1.0F, 0.0F, 1.0F, 0.0F},
	{1.0F, 1.0F, 0.0F, 1.0F, 1.0F},
	{-1.0F, 1.0F, 0.0F, 0.0F, 1.0F}
};

constexpr int quad_indices[] = {0, 1, 2, 2, 3, 0};

camera_t camera
{
	perspective(90.0F, static_cast<float>(screen_width) / screen_height, 0.01F, 100000.0F),
	{
		{
			0.0F, 10.0F, 0.0F, 1.0F
		},
		{}, {1.0F, 1.0F, 1.0F}
	}
};

std::unordered_map<int, bool> buttons{};

bool  first_mouse{true};
float x{};
float y{};
float x_offset{};
float y_offset{};
bool  options{};
int   radio_button_value{3};
float noise_scale{25000.0F};
float scattering_factor = 0.001F;
float extinction_factor = 0.001F;
float sun_intensity = 5.0F;

static void mouse_callback(GLFWwindow* /*window*/, double x_pos, double y_pos);
static void key_callback(GLFWwindow* window, int key, int /*scan_code*/, int action, int /*mode*/);
static void process_input(float dt);

int main()
{
	// init glfw
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

	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwMakeContextCurrent(window);
	glfwSwapInterval(0); // disable v-sync
	glbinding::initialize(glfwGetProcAddress);
	gl::glViewport(0, 0, screen_width, screen_height);
	log_opengl_error();

	//init imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsLight();
	auto& io = ImGui::GetIO();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");


	// load textures
	stbi_set_flip_vertically_on_load(1);
	gl::GLuint cloud_base_texture; // load base cloud shape texture

	{
		log("loading base cloud shape texture");

		int        width;
		int        height;
		int        number_of_components;
		const auto cloud_base_image = stbi_load("textures/noise_shape2.tga", &width, &height, &number_of_components, 0);
		//const auto cloud_base_image = stbi_load("textures/LowFrequency3DTexture.tga", &width, &height, &number_of_components, 0);

		gl::glGenTextures(1, &cloud_base_texture);
		glBindTexture(gl::GLenum::GL_TEXTURE_3D, cloud_base_texture);
		glTexParameteri(gl::GLenum::GL_TEXTURE_3D, gl::GLenum::GL_TEXTURE_MIN_FILTER, gl::GLenum::GL_LINEAR);
		glTexParameteri(gl::GLenum::GL_TEXTURE_3D, gl::GLenum::GL_TEXTURE_MAG_FILTER, gl::GLenum::GL_LINEAR);
		glTexImage3D(gl::GLenum::GL_TEXTURE_3D, 0, gl::GLenum::GL_RGBA8, 128, 128, 128, 0, gl::GLenum::GL_RGBA,
		             gl::GLenum::GL_UNSIGNED_BYTE, cloud_base_image);

		log_opengl_error();
		stbi_image_free(cloud_base_image);
	}

	gl::GLuint cloud_erosion_texture; // load cloud erosion texture
	{
		log("loading cloud erosion texture");

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

		log_opengl_error();
		stbi_image_free(cloud_erosion_image);
	}

	// load weather map
	gl::GLuint weather_map_texture;
	{
		log("loading weather map texture");

		int        width;
		int        height;
		int        number_of_components;
		const auto weather_map_data = stbi_load("textures/weather_map.png", &width, &height, &number_of_components,
		                                        0);
		gl::glGenTextures(1, &weather_map_texture);
		glBindTexture(gl::GLenum::GL_TEXTURE_2D, weather_map_texture);
		glTexParameteri(gl::GLenum::GL_TEXTURE_2D, gl::GLenum::GL_TEXTURE_MIN_FILTER, gl::GLenum::GL_LINEAR);
		glTexParameteri(gl::GLenum::GL_TEXTURE_2D, gl::GLenum::GL_TEXTURE_MAG_FILTER, gl::GLenum::GL_LINEAR);
		glTexParameteri(gl::GLenum::GL_TEXTURE_2D, gl::GLenum::GL_TEXTURE_WRAP_S, gl::GLenum::GL_REPEAT);

		glTexParameteri(gl::GLenum::GL_TEXTURE_2D, gl::GLenum::GL_TEXTURE_WRAP_T, gl::GLenum::GL_REPEAT);
		glTexImage2D(gl::GLenum::GL_TEXTURE_2D, 0, gl::GLenum::GL_RGBA8, 520, 520, 0, gl::GLenum::GL_RGBA,
		             gl::GLenum::GL_UNSIGNED_BYTE, weather_map_data);

		log_opengl_error();
		stbi_image_free(weather_map_data);
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
	glBufferData(gl::GLenum::GL_ARRAY_BUFFER, 20 * sizeof(float), full_screen_quad, gl::GLenum::GL_STATIC_DRAW);
	glBindBuffer(gl::GLenum::GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(gl::GLenum::GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(int), quad_indices, gl::GLenum::GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, gl::GLenum::GL_FLOAT, false, 5 * sizeof(float), reinterpret_cast<void*>(0));
	glVertexAttribPointer(1, 2, gl::GLenum::GL_FLOAT, false, 5 * sizeof(float),
	                      reinterpret_cast<void*>(3 * sizeof(float)));
	gl::glEnableVertexAttribArray(0);
	gl::glEnableVertexAttribArray(1);
	log_opengl_error();

	gl::GLuint cloud_shader;
	// load cloud shader
	{
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

		cloud_shader = gl::glCreateProgram();
		gl::glAttachShader(cloud_shader, vertex);
		gl::glAttachShader(cloud_shader, fragment);
		gl::glLinkProgram(cloud_shader);

		gl::glDeleteShader(vertex);
		gl::glDeleteShader(fragment);
	}

	// load raymarching shader
	gl::GLuint raymarching_shader;
	{
		std::string vertex_code{};
		std::string fragment_code{};

		std::ifstream vertex_file{"raymarch_vertex.glsl"};
		std::ifstream fragment_file{"raymarch_fragment.glsl"};

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

		raymarching_shader = gl::glCreateProgram();
		gl::glAttachShader(raymarching_shader, vertex);
		gl::glAttachShader(raymarching_shader, fragment);
		gl::glLinkProgram(raymarching_shader);

		gl::glDeleteShader(vertex);
		gl::glDeleteShader(fragment);
	}

	gl::glClearColor(0.0F, 0.0F, 0.0F, 1.0F);

	log_opengl_error();

	// create framebuffers
	framebuffer_t framebuffer{screen_width, screen_height, 1, true};

	auto delta_time = 1.0F / 60.0F;

	while (glfwWindowShouldClose(window) == 0)
	{
		glfwPollEvents();

		//std::stringstream ss{};
		//ss << "camera position: " << camera.transform.position.x << ", " << camera.transform.position.y << ", " <<
		//	camera.transform.position.z;
		//log(ss.str());

		auto start = std::chrono::high_resolution_clock::now();
		process_input(delta_time);

		// draw basic quad
		framebuffer.bind();
		glClear(gl::ClearBufferMask::GL_COLOR_BUFFER_BIT | gl::ClearBufferMask::GL_DEPTH_BUFFER_BIT);
		gl::glUseProgram(cloud_shader);
		set_uniform(cloud_shader, "projection", camera.projection);
		set_uniform(cloud_shader, "view", camera.transform.get_view_matrix());
		auto transform_matrix = scale(30000);
		transform_matrix *= rotate(radians(90), {1.0F, 0.0F, 0.0F});
		set_uniform(cloud_shader, "model", transform_matrix);
		glDrawElements(gl::GLenum::GL_TRIANGLES, 6, gl::GLenum::GL_UNSIGNED_INT, nullptr);

		framebuffer_t::unbind();

		// raymarching

		glClear(gl::ClearBufferMask::GL_COLOR_BUFFER_BIT | gl::ClearBufferMask::GL_DEPTH_BUFFER_BIT);
		gl::glUseProgram(raymarching_shader);
		framebuffer.colour_attachments.front().bind(0);
		glActiveTexture(gl::GLenum::GL_TEXTURE1);
		glBindTexture(gl::GLenum::GL_TEXTURE_3D, cloud_base_texture);
		glActiveTexture(gl::GLenum::GL_TEXTURE2);
		glBindTexture(gl::GLenum::GL_TEXTURE_3D, cloud_erosion_texture);
		glActiveTexture(gl::GLenum::GL_TEXTURE3);
		glBindTexture(gl::GLenum::GL_TEXTURE_2D, weather_map_texture);
		set_uniform(raymarching_shader, "full_screen", 0);
		set_uniform(raymarching_shader, "cloud_base", 1);
		set_uniform(raymarching_shader, "cloud_erosion", 2);
		set_uniform(raymarching_shader, "weather_map", 3);
		set_uniform(raymarching_shader, "projection", camera.projection);
		set_uniform(raymarching_shader, "view", camera.transform.get_view_matrix());
		set_uniform(raymarching_shader, "camera_pos", glm::vec3{camera.transform.position});
		set_uniform(raymarching_shader, "weather_map_visualization", static_cast<float>(radio_button_value == 1));
		set_uniform(raymarching_shader, "low_frequency_noise_visualization",
		            static_cast<float>(radio_button_value == 2));
		set_uniform(raymarching_shader, "high_frequency_noise_visualization",
		            static_cast<float>(radio_button_value == 3));
		set_uniform(raymarching_shader, "noise_scale", noise_scale);
		set_uniform(raymarching_shader, "scattering_factor", scattering_factor);
		set_uniform(raymarching_shader, "extinction_factor", extinction_factor);
		set_uniform(raymarching_shader, "sun_intensity", sun_intensity);
		glDrawElements(gl::GLenum::GL_TRIANGLES, 6, gl::GLenum::GL_UNSIGNED_INT, nullptr);

		// render gui
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("clouds");
		ImGui::Text("average fps: %.2f fps", ImGui::GetIO().Framerate);
		ImGui::Text("average frametime: %.2f ms", 1000.0F / ImGui::GetIO().Framerate);
		ImGui::Text("camera world position: x=%f, y=%f, z=%f",
		            camera.transform.position.x,
		            camera.transform.position.y,
		            camera.transform.position.z);
		ImGui::Text("press 'o' to toggle options");
		ImGui::End();

		if (options)
		{
			ImGui::Begin("options");

			ImGui::RadioButton("weather map visualization", &radio_button_value, 1);
			ImGui::RadioButton("low frequency noise", &radio_button_value, 2);
			ImGui::RadioButton("high frequency noise", &radio_button_value, 3);

			ImGui::SliderFloat("noise scale", &noise_scale, 1000.0F, 100000.0F, "%.0f");
			ImGui::NewLine();

			ImGui::SliderFloat("scattering factor", &scattering_factor, 0.000001F, 0.01F, "%.5f");
			ImGui::NewLine();

			ImGui::SliderFloat("extinction factor", &extinction_factor, 0.000001F, 0.01F, "%.5f");
			ImGui::NewLine();

			ImGui::SliderFloat("sun intensity", &sun_intensity, 1.0F, 100.0F, "%.5f");
			ImGui::NewLine();

			ImGui::End();
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		auto end   = std::chrono::high_resolution_clock::now();
		delta_time = std::chrono::duration<float>(end - start).count();


		glfwSwapBuffers(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();

	glfwDestroyWindow(window);
	glfwTerminate();
}

static void process_input(float dt)
{
	if (options)
	{
		return;
	}

	auto view_matrix = camera.transform.get_view_matrix();

	const auto right   = glm::vec3{view_matrix[0][0], view_matrix[1][0], view_matrix[2][0]};
	const auto up      = glm::vec3{view_matrix[0][1], view_matrix[1][1], view_matrix[2][1]};
	const auto forward = -glm::vec3{view_matrix[0][2], view_matrix[1][2], view_matrix[2][2]};


	glm::vec3 trans{};

	if (buttons[GLFW_KEY_W])
	{
		trans += forward * dt * camera.movement_speed;
	}

	if (buttons[GLFW_KEY_S])
	{
		trans -= forward * dt * camera.movement_speed;
	}

	if (buttons[GLFW_KEY_D])
	{
		trans += right * dt * camera.movement_speed;
	}

	if (buttons[GLFW_KEY_A])
	{
		trans -= right * dt * camera.movement_speed;
	}

	if (buttons[GLFW_KEY_Q])
	{
		trans += up * dt * camera.movement_speed;
	}

	if (buttons[GLFW_KEY_E])
	{
		trans -= up * dt * camera.movement_speed;
	}

	camera.transform.position = translate(trans) * camera.transform.position;

	if (x_offset != 0.0F)
	{
		camera.transform.rotation.y -= x_offset * camera.mouse_sensitivity;
		camera.transform.rotation.y = std::fmod(camera.transform.rotation.y, 360.0F);
		x_offset                    = 0.0F;
	}

	if (y_offset != 0.0F)
	{
		camera.transform.rotation.x -= y_offset * camera.mouse_sensitivity;
		camera.transform.rotation.x = std::clamp(camera.transform.rotation.x, -89.0F, 89.0F);
		y_offset                    = 0.0F;
	}
}

static void key_callback(GLFWwindow* window, int key, int /*scan_code*/, int action, int /*mode*/)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, 1);
		return;
	}

	if (key == GLFW_KEY_O && action == GLFW_PRESS)
	{
		if (!options)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		else
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		options = !options;
	}
	else if (action == GLFW_PRESS)
	{
		buttons[key] = true;
	}
	else if (action == GLFW_RELEASE)
	{
		buttons[key] = false;
	}
}

static void mouse_callback(GLFWwindow* /*window*/, double x_pos, double y_pos)
{
	if (first_mouse)
	{
		x           = static_cast<float>(x_pos);
		y           = static_cast<float>(y_pos);
		first_mouse = false;
	}

	x_offset = static_cast<float>(x_pos) - x;
	y_offset = static_cast<float>(y_pos) - y;

	x = static_cast<float>(x_pos);
	y = static_cast<float>(y_pos);
}
