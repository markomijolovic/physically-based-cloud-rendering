#define GLFW_INCLUDE_NONE

#include <fstream>

#include <sstream>

#include <chrono>

#include <iostream>

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
float low_freq_noise_scale{25000.0F};
float high_freq_noise_scale{1000.0F};
float scattering_factor = 0.020F;
float extinction_factor = 0.025F;
float sun_intensity = 15.0F;
float high_freq_noise_factor = 1.0F;
bool multiple_scattering_approximation{ true };
int N{ 8 };
float a{0.5F};
float b{ 0.5F };
float c{ 0.5F };
int primary_ray_steps{ 128 };
int secondary_ray_steps{ 16 };
float cumulative_time{};
float cloud_speed{ 0.1F };
glm::vec3 wind_direction{1.0F, 0.0F, 0.0F};
glm::vec3 wind_direction_normalized{};
glm::vec3 sun_direction{ -1.0F, -1.0F, 0.0F };
glm::vec3 sun_direction_normalized{};

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
		//glTexImage2D(gl::GLenum::GL_TEXTURE_2D, 0, gl::GLenum::GL_RGBA8, width, height, 0, gl::GLenum::GL_RGBA,
		//             gl::GLenum::GL_UNSIGNED_BYTE, weather_map_data);
		glTexImage2D(gl::GLenum::GL_TEXTURE_2D, 0, gl::GLenum::GL_RGBA8, width, height, 0, gl::GLenum::GL_RGBA,
		             gl::GLenum::GL_UNSIGNED_BYTE, weather_map_data);
		log_opengl_error();
		stbi_image_free(weather_map_data);
	}

	// load blue noise texture
	gl::GLuint blue_noise_texture;
	{
		log("loading blue noise texture");

		int        width;
		int        height;
		int        number_of_components;
		const auto weather_map_data = stbi_load("textures/blue_noise.png", &width, &height, &number_of_components,
			0);
		gl::glGenTextures(1, &blue_noise_texture);
		glBindTexture(gl::GLenum::GL_TEXTURE_2D, blue_noise_texture);
		glTexParameteri(gl::GLenum::GL_TEXTURE_2D, gl::GLenum::GL_TEXTURE_MIN_FILTER, gl::GLenum::GL_LINEAR);
		glTexParameteri(gl::GLenum::GL_TEXTURE_2D, gl::GLenum::GL_TEXTURE_MAG_FILTER, gl::GLenum::GL_LINEAR);
		glTexParameteri(gl::GLenum::GL_TEXTURE_2D, gl::GLenum::GL_TEXTURE_WRAP_S, gl::GLenum::GL_REPEAT);

		glTexParameteri(gl::GLenum::GL_TEXTURE_2D, gl::GLenum::GL_TEXTURE_WRAP_T, gl::GLenum::GL_REPEAT);
		//glTexImage2D(gl::GLenum::GL_TEXTURE_2D, 0, gl::GLenum::GL_RGBA8, width, height, 0, gl::GLenum::GL_RGBA,
		//             gl::GLenum::GL_UNSIGNED_BYTE, weather_map_data);
		glTexImage2D(gl::GLenum::GL_TEXTURE_2D, 0, gl::GLenum::GL_RGBA8, width, height, 0, gl::GLenum::GL_RGBA,
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


	// load blur shader
	gl::GLuint blur_shader;
	{
		std::string vertex_code{};
		std::string fragment_code{};

		std::ifstream vertex_file{ "raymarch_vertex.glsl" };

		std::ifstream fragment_file{ "blur_fragment.glsl" };

		std::stringstream vertex_stream{};
		std::stringstream fragment_stream{};
		vertex_stream << vertex_file.rdbuf();
		fragment_stream << fragment_file.rdbuf();

		vertex_code = vertex_stream.str();
		fragment_code = fragment_stream.str();

		auto p_vertex_data = vertex_code.data();
		auto p_fragment_data = fragment_code.data();

		auto vertex = glCreateShader(gl::GLenum::GL_VERTEX_SHADER);
		gl::glShaderSource(vertex, 1, &p_vertex_data, nullptr);
		gl::glCompileShader(vertex);

		auto fragment = glCreateShader(gl::GLenum::GL_FRAGMENT_SHADER);
		gl::glShaderSource(fragment, 1, &p_fragment_data, nullptr);
		gl::glCompileShader(fragment);

		blur_shader = gl::glCreateProgram();
		gl::glAttachShader(blur_shader, vertex);
		gl::glAttachShader(blur_shader, fragment);
		gl::glLinkProgram(blur_shader);

		gl::glDeleteShader(vertex);
		gl::glDeleteShader(fragment);
	}

	// load blur shader
	gl::GLuint tonemap_shader;
	{
		std::string vertex_code{};
		std::string fragment_code{};

		std::ifstream vertex_file{ "raymarch_vertex.glsl" };

		std::ifstream fragment_file{ "tonemap_fragment.glsl" };

		std::stringstream vertex_stream{};
		std::stringstream fragment_stream{};
		vertex_stream << vertex_file.rdbuf();
		fragment_stream << fragment_file.rdbuf();

		vertex_code = vertex_stream.str();
		fragment_code = fragment_stream.str();

		auto p_vertex_data = vertex_code.data();
		auto p_fragment_data = fragment_code.data();

		auto vertex = glCreateShader(gl::GLenum::GL_VERTEX_SHADER);
		gl::glShaderSource(vertex, 1, &p_vertex_data, nullptr);
		gl::glCompileShader(vertex);

		auto fragment = glCreateShader(gl::GLenum::GL_FRAGMENT_SHADER);
		gl::glShaderSource(fragment, 1, &p_fragment_data, nullptr);
		gl::glCompileShader(fragment);

		tonemap_shader = gl::glCreateProgram();
		gl::glAttachShader(tonemap_shader, vertex);
		gl::glAttachShader(tonemap_shader, fragment);
		gl::glLinkProgram(tonemap_shader);

		gl::glDeleteShader(vertex);
		gl::glDeleteShader(fragment);
	}


	gl::glClearColor(0.0F, 0.0F, 0.0F, 1.0F);

	log_opengl_error();

	// create framebuffers
	framebuffer_t framebuffer{screen_width, screen_height, 1, true};
	framebuffer_t framebuffer2{screen_width, screen_height, 1, true};
	framebuffer_t framebuffer3{screen_width, screen_height, 1, true};

	auto delta_time = 1.0F / 60.0F;
	auto now = std::chrono::high_resolution_clock::now();

	while (glfwWindowShouldClose(window) == 0)
	{
		glfwPollEvents();

		//std::stringstream ss{};
		//ss << "camera position: " << camera.transform.position.x << ", " << camera.transform.position.y << ", " <<
		//	camera.transform.position.z;
		//log(ss.str());

		delta_time = std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock::now() - now).count();
		now = std::chrono::high_resolution_clock::now();
		cumulative_time += delta_time;
		
		process_input(delta_time);
		
		std::cout << "Delta time: " << delta_time << std::endl;

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

		framebuffer2.bind();
		glClear(gl::ClearBufferMask::GL_COLOR_BUFFER_BIT | gl::ClearBufferMask::GL_DEPTH_BUFFER_BIT);
		gl::glUseProgram(raymarching_shader);
		framebuffer.colour_attachments.front().bind(0);
		glActiveTexture(gl::GLenum::GL_TEXTURE1);
		glBindTexture(gl::GLenum::GL_TEXTURE_3D, cloud_base_texture);
		glActiveTexture(gl::GLenum::GL_TEXTURE2);
		glBindTexture(gl::GLenum::GL_TEXTURE_3D, cloud_erosion_texture);
		glActiveTexture(gl::GLenum::GL_TEXTURE3);
		glBindTexture(gl::GLenum::GL_TEXTURE_2D, weather_map_texture);
		glActiveTexture(gl::GLenum::GL_TEXTURE4);
		glBindTexture(gl::GLenum::GL_TEXTURE_2D, blue_noise_texture);
		set_uniform(raymarching_shader, "full_screen", 0);
		set_uniform(raymarching_shader, "cloud_base", 1);
		set_uniform(raymarching_shader, "cloud_erosion", 2);
		set_uniform(raymarching_shader, "weather_map", 3);
		set_uniform(raymarching_shader, "blue_noise", 4);
		set_uniform(raymarching_shader, "projection", camera.projection);
		set_uniform(raymarching_shader, "view", camera.transform.get_view_matrix());
		set_uniform(raymarching_shader, "camera_pos", glm::vec3{camera.transform.position});
		set_uniform(raymarching_shader, "weather_map_visualization", static_cast<int>(radio_button_value == 1));
		set_uniform(raymarching_shader, "low_frequency_noise_visualization",
		            static_cast<int>(radio_button_value == 2));
		set_uniform(raymarching_shader, "high_frequency_noise_visualization",
		            static_cast<int>(radio_button_value == 3));
		set_uniform(raymarching_shader, "multiple_scattering_approximation", static_cast<int>(multiple_scattering_approximation));
		set_uniform(raymarching_shader, "low_freq_noise_scale", low_freq_noise_scale);
		set_uniform(raymarching_shader, "high_freq_noise_scale", high_freq_noise_scale);
		set_uniform(raymarching_shader, "scattering_factor", scattering_factor);
		set_uniform(raymarching_shader, "extinction_factor", extinction_factor);
		set_uniform(raymarching_shader, "sun_intensity", sun_intensity);
		set_uniform(raymarching_shader, "high_freq_noise_factor", high_freq_noise_factor);
		set_uniform(raymarching_shader, "N", N);
		set_uniform(raymarching_shader, "a", a);
		set_uniform(raymarching_shader, "b", b);
		set_uniform(raymarching_shader, "c", c);
		set_uniform(raymarching_shader, "primary_ray_steps", primary_ray_steps);
		set_uniform(raymarching_shader, "secondary_ray_steps", secondary_ray_steps);
		set_uniform(raymarching_shader, "time", cumulative_time);
		set_uniform(raymarching_shader, "cloud_speed", cloud_speed);
		wind_direction_normalized = normalize(wind_direction);
		set_uniform(raymarching_shader, "wind_direction", wind_direction_normalized);
		sun_direction_normalized = normalize(sun_direction);
		set_uniform(raymarching_shader, "sun_direction", sun_direction_normalized);
		glDrawElements(gl::GLenum::GL_TRIANGLES, 6, gl::GLenum::GL_UNSIGNED_INT, nullptr);


		bool horizontal = true;
		int amount = 4;
		gl::glUseProgram(blur_shader);
		set_uniform(blur_shader, "full_screen", 0);
		for (unsigned int i = 0; i < amount; i++)
		{
			if (horizontal)
			{
				framebuffer3.bind();
				framebuffer2.colour_attachments.front().bind(0);
			}
			else
			{
				framebuffer2.bind();
				framebuffer3.colour_attachments.front().bind(0);
			}
			set_uniform(blur_shader, "horizontal", horizontal);

			glDrawElements(gl::GLenum::GL_TRIANGLES, 6, gl::GLenum::GL_UNSIGNED_INT, nullptr);
			horizontal = !horizontal;
		}
		
			
		//framebuffer3.bind();
		//glClear(gl::ClearBufferMask::GL_COLOR_BUFFER_BIT | gl::ClearBufferMask::GL_DEPTH_BUFFER_BIT);
		//gl::glUseProgram(blur_shader);

		//framebuffer2.colour_attachments.front().bind(0);
		//set_uniform(blur_shader, "full_screen", 0);
		//set_uniform(blur_shader, "horizontal", 0);

		//glDrawElements(gl::GLenum::GL_TRIANGLES, 6, gl::GLenum::GL_UNSIGNED_INT, nullptr);

		//framebuffer2.bind();
		//glClear(gl::ClearBufferMask::GL_COLOR_BUFFER_BIT | gl::ClearBufferMask::GL_DEPTH_BUFFER_BIT);
		//framebuffer3.colour_attachments.front().bind(0);
		//set_uniform(blur_shader, "full_screen", 0);
		//set_uniform(blur_shader, "horizontal", 1);

		//glDrawElements(gl::GLenum::GL_TRIANGLES, 6, gl::GLenum::GL_UNSIGNED_INT, nullptr);

		framebuffer_t::unbind();
		glClear(gl::ClearBufferMask::GL_COLOR_BUFFER_BIT | gl::ClearBufferMask::GL_DEPTH_BUFFER_BIT);
		gl::glUseProgram(tonemap_shader);

		framebuffer2.colour_attachments.front().bind(0);
		set_uniform(tonemap_shader, "full_screen", 0);
		glDrawElements(gl::GLenum::GL_TRIANGLES, 6, gl::GLenum::GL_UNSIGNED_INT, nullptr);

		// render gui
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("clouds");
		ImGui::Text("average fps: %.2f fps", ImGui::GetIO().Framerate);
		ImGui::Text("average frametime: %.2f ms", 1000.0F / ImGui::GetIO().Framerate);
		ImGui::Text("time elapsed: %.2f ms", cumulative_time);
		ImGui::Text("camera world position: x=%f, y=%f, z=%f",
		            camera.transform.position.x,
		            camera.transform.position.y,
		            camera.transform.position.z);
		ImGui::Text("press 'o' to toggle options");
		ImGui::End();

		if (options)
		{
			ImGui::Begin("options");

			ImGui::SliderInt("number of primary ray steps", &primary_ray_steps, 1, 500, "%d");
			ImGui::NewLine();
			
			ImGui::SliderInt("number of secondary ray steps", &secondary_ray_steps, 1, 100, "%d");
			ImGui::NewLine();

			ImGui::RadioButton("weather map visualization", &radio_button_value, 1);
			ImGui::RadioButton("low frequency noise", &radio_button_value, 2);
			ImGui::RadioButton("high frequency noise", &radio_button_value, 3);

			ImGui::SliderFloat("low frequency noise scale", &low_freq_noise_scale, 1000.0F, 100000.0F, "%.0f");
			ImGui::NewLine();
			
			ImGui::SliderFloat("high frequency noise scale", &high_freq_noise_scale, 10.0F, 10000.0F, "%.0f");
			ImGui::NewLine();
			
			ImGui::SliderFloat("high frequency noise factor", &high_freq_noise_factor, 0.0F, 1.0F, "%.5f");
			ImGui::NewLine();

			ImGui::SliderFloat("scattering factor", &scattering_factor, 0.001F, 0.1F, "%.5f");
			ImGui::NewLine();

			ImGui::SliderFloat("extinction factor", &extinction_factor, 0.001F, 0.1F, "%.5f");
			ImGui::NewLine();

			ImGui::SliderFloat3("wind direction", &wind_direction[0], -1.0F, 1.0F, "%.5f");
			ImGui::NewLine();

			ImGui::SliderFloat3("sun direction", &sun_direction[0], -1.0F, 1.0F, "%.5f");
			ImGui::NewLine();
			
			ImGui::SliderFloat("cloud speed", &cloud_speed, 0.0F, 10.0F, "%.5f");
			ImGui::NewLine();

			ImGui::SliderFloat("sun illuminance", &sun_intensity, 10.0F, 200.0F, "%.5f");
			ImGui::NewLine();

			ImGui::Checkbox("multiple scattering approximation", &multiple_scattering_approximation);
			ImGui::NewLine();

			ImGui::SliderInt("octaves count", &N, 1, 8, "%d");
			ImGui::NewLine();

			ImGui::SliderFloat("attenuation", &a, 0.01F, 1.0F, "%.2f");
			ImGui::NewLine();

			ImGui::SliderFloat("contribution", &b, 0.01F, 1.0F, "%.2f");
			ImGui::NewLine();

			ImGui::SliderFloat("eccentricity attenuation", &c, 0.01F, 1.0F, "%.2f");
			ImGui::NewLine();
			
			ImGui::End();
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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
