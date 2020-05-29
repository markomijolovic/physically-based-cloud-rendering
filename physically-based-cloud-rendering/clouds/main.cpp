#define GLFW_INCLUDE_NONE

#define TINYEXR_IMPLEMENTATION

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stb_image_write.h>

#include "tinyexr.h"

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

#include "preetham.hpp"

constexpr auto screen_width = 1280;
constexpr auto screen_height = 720;

constexpr float full_screen_quad[][5] =
{
	{-1.0F, -1.0F, 0.0F, 0.0F, 0.0F},
	{1.0F, -1.0F, 0.0F, 1.0F, 0.0F},
	{1.0F, 1.0F, 0.0F, 1.0F, 1.0F},
	{-1.0F, 1.0F, 0.0F, 0.0F, 1.0F}
};

constexpr int quad_indices[] = { 0, 1, 2, 2, 3, 0 };

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

bool  first_mouse{ true };
float x{};
float y{};
float x_offset{};
float y_offset{};
bool  options{};
int   radio_button_value{ 3 };
float low_freq_noise_scale{ 7000.0F };
float high_freq_noise_scale{ 1750.0F };
float weather_map_scale{ 30000.0F };
float scattering_factor = 0.025F;
float extinction_factor = 0.025F;
float sun_intensity = 1.0F;
float global_cloud_coverage = 1.0F;
float high_freq_noise_factor = 0.5F;
float anvil_bias{ 1.0F };
float exposure_factor{ 0.00001F };
float turbidity{ 5.0F };
bool multiple_scattering_approximation{ true };
bool blue_noise{};
bool blur{};
bool weather_map{ true };
bool ambient{ true };
int N{ 8 };
float a{ 0.5F };
float b{ 0.5F };
float c{ 0.5F };
int primary_ray_steps{ 64 };
int secondary_ray_steps{ 16 };
float cumulative_time{};
float cloud_speed{ 0.0F };
glm::vec3 wind_direction{ 1.0F, 0.0F, 0.0F };
glm::vec3 wind_direction_normalized{};
glm::vec3 sun_direction{ 0.0F, -1.0F, 0.0F };
glm::vec3 sun_direction_normalized{};

static void mouse_callback(GLFWwindow* /*window*/, double x_pos, double y_pos);
static void key_callback(GLFWwindow* window, int key, int /*scan_code*/, int action, int /*mode*/);
static void process_input(float dt);

void generate_mie()
{

	//gl::GLuint mie_texture; // mie phase function
	//{

	//	EXRHeader header;
	//	InitEXRHeader(&header);

	//	EXRImage image;
	//	InitEXRImage(&image);

	//	image.num_channels = 4;

	//	
	//	const char* input = "textures/mie_phase_function.exr";
	//	float* out = (float*)malloc(1800 * 4 * sizeof(float));
	//	
	//	for (auto i = 0; i < 1800; i++)
	//	{
	//		out[4 * i] = mie_cloud_reference[i].x;
	//		out[4 * i + 1] = mie_cloud_reference[i].y;
	//		out[4 * i + 2] = mie_cloud_reference[i].z;
	//		out[4 * i + 3] = 1;
	//		if (i <= 78)
	//		{
	//			out[4 * i] += chopped_peak[i];
	//			out[4 * i + 1] += chopped_peak[i];
	//			out[4 * i + 2] += chopped_peak[i];
	//		}
	//		std::cout << out[4*i] << " " << out[4*i + 1] << " " << out[4*i+2] << std::endl;
	//	}

	//	stbi_write_hdr("textures/mie_phase_function.hdr", 1800, 1, 4, out);
	//	
	//	free(out);
	//}
}

void test_normalization()
{
	/*log("loading cloud erosion texture");

	int        width;
	int        height;
	int        number_of_components;
	auto out = stbi_loadf("textures/mie_phase_function_normalized.hdr", &width, &height, &number_of_components,
		0);

	float r{};
	float g{};
	float b{};

	for (auto i = 0; i < 1800; i++)
	{
		r += 2*pi*out[3 * i]* 0.1F * pi/180.0F*sin((i+1)*0.1F*pi/180.0F);
		g += 2*pi*out[3 * i + 1] * 0.1F * pi / 180.0F * sin((i + 1) * 0.1F * pi / 180.0F);
		b += 2*pi*out[3 * i + 2] * 0.1F * pi / 180.0F * sin((i + 1) * 0.1F * pi / 180.0F);
	}

	std::cout << 4 * pi << std::endl;
	std::cout << r << std::endl;
	std::cout << g << std::endl;
	std::cout << b << std::endl;
	
	return;
	auto normalized = (float*)malloc(1800 * 4 * sizeof(float));

	EXRHeader header;
	InitEXRHeader(&header);

	EXRImage image;
	InitEXRImage(&image);

	image.num_channels = 4;

	for (auto i = 0; i < 1800; i++)
	{
		normalized[4 * i] = out[3 * i] / r;
		normalized[4 * i+1] = out[3 * i+1] / g;
		normalized[4 * i+2] = out[3 * i+2] / b;
		normalized[4 * i+3] = 1;
	}

	stbi_write_hdr("textures/mie_phase_function_normalized.hdr", 1800, 1, 4, normalized);
	free(normalized);
	
	stbi_image_free(out);*/
}

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
		const auto cloud_base_image = stbi_load("textures/noise_shape.tga", &width, &height, &number_of_components, 0);

		gl::glGenTextures(1, &cloud_base_texture);
		glBindTexture(gl::GLenum::GL_TEXTURE_3D, cloud_base_texture);
		float aniso{};
		glGetFloatv(gl::GLenum::GL_MAX_TEXTURE_MAX_ANISOTROPY, &aniso);
		glTexParameterf(gl::GLenum::GL_TEXTURE_3D, gl::GLenum::GL_TEXTURE_MAX_ANISOTROPY, aniso);
		glTexParameteri(gl::GLenum::GL_TEXTURE_3D, gl::GLenum::GL_TEXTURE_MIN_FILTER, gl::GLenum::GL_LINEAR);
		glTexParameteri(gl::GLenum::GL_TEXTURE_3D, gl::GLenum::GL_TEXTURE_MAG_FILTER, gl::GLenum::GL_LINEAR);

		const auto levels = static_cast<uint32_t>(floor(log2(128))) + 1;
		gl::glTexStorage3D(gl::GLenum::GL_TEXTURE_3D, levels, gl::GLenum::GL_RGBA8, 128, 128, 128);

		glTexSubImage3D(gl::GLenum::GL_TEXTURE_3D, 0, 0, 0, 0, 128, 128, 128, gl::GLenum::GL_RGBA, gl::GLenum::GL_UNSIGNED_BYTE, cloud_base_image);

		log_opengl_error();
		stbi_image_free(cloud_base_image);
	}

	gl::GLuint cloud_erosion_texture; // load cloud erosion texture
	{
		log("loading cloud erosion texture");

		int        width;
		int        height;
		int        number_of_components;
		const auto cloud_erosion_image = stbi_load("textures/noise_erosion_hd.tga", &width, &height, &number_of_components,
			0);

		gl::glGenTextures(1, &cloud_erosion_texture);

		const auto levels = static_cast<uint32_t>(floor(log2(64))) + 1;

		glBindTexture(gl::GLenum::GL_TEXTURE_3D, cloud_erosion_texture);
		float aniso{};
		glGetFloatv(gl::GLenum::GL_MAX_TEXTURE_MAX_ANISOTROPY, &aniso);
		glTexParameterf(gl::GLenum::GL_TEXTURE_3D, gl::GLenum::GL_TEXTURE_MAX_ANISOTROPY, aniso);
		glTexParameteri(gl::GLenum::GL_TEXTURE_3D, gl::GLenum::GL_TEXTURE_MIN_FILTER, gl::GLenum::GL_LINEAR);
		glTexParameteri(gl::GLenum::GL_TEXTURE_3D, gl::GLenum::GL_TEXTURE_MAG_FILTER, gl::GLenum::GL_LINEAR);

		gl::glTexStorage3D(gl::GLenum::GL_TEXTURE_3D, levels, gl::GLenum::GL_RGBA8, 64, 64, 64);

		glTexSubImage3D(gl::GLenum::GL_TEXTURE_3D, 0, 0, 0, 0, 64, 64, 64, gl::GLenum::GL_RGBA, gl::GLenum::GL_UNSIGNED_BYTE, cloud_erosion_image);

		glGenerateMipmap(gl::GLenum::GL_TEXTURE_3D);


		log_opengl_error();
		stbi_image_free(cloud_erosion_image);
	}

	gl::GLuint mie_texture; // mie phase function
	{
		log("loading cloud erosion texture");

		int        width;
		int        height;
		int        number_of_components;
		const auto out = stbi_loadf("textures/mie_phase_function_normalized.hdr", &width, &height, &number_of_components,
			0);

		gl::glGenTextures(1, &mie_texture);

		glBindTexture(gl::GLenum::GL_TEXTURE_1D, mie_texture);
		glTexParameteri(gl::GLenum::GL_TEXTURE_1D, gl::GLenum::GL_TEXTURE_MIN_FILTER, gl::GLenum::GL_LINEAR);
		glTexParameteri(gl::GLenum::GL_TEXTURE_1D, gl::GLenum::GL_TEXTURE_MAG_FILTER, gl::GLenum::GL_LINEAR);
		const auto levels = static_cast<uint32_t>(floor(log2(1800))) + 1;

		gl::glTexStorage1D(gl::GLenum::GL_TEXTURE_1D, levels, gl::GLenum::GL_RGB32F, 1800);
		glTexSubImage1D(gl::GLenum::GL_TEXTURE_1D, 0, 0, width, gl::GLenum::GL_RGB,
			gl::GLenum::GL_FLOAT, out);
		log_opengl_error();
		stbi_image_free(out);
	}

	// load weather map
	gl::GLuint weather_map_texture;
	{
		log("loading weather map texture");

		int        width;
		int        height;
		int        number_of_components;
		const auto weather_map_data = stbi_load("textures/perlin_test_cumulus.tga", &width, &height, &number_of_components,
			0);
		gl::glGenTextures(1, &weather_map_texture);
		glBindTexture(gl::GLenum::GL_TEXTURE_2D, weather_map_texture);
		glTexParameteri(gl::GLenum::GL_TEXTURE_2D, gl::GLenum::GL_TEXTURE_MIN_FILTER, gl::GLenum::GL_LINEAR);
		glTexParameteri(gl::GLenum::GL_TEXTURE_2D, gl::GLenum::GL_TEXTURE_MAG_FILTER, gl::GLenum::GL_LINEAR);
		glTexParameteri(gl::GLenum::GL_TEXTURE_2D, gl::GLenum::GL_TEXTURE_WRAP_S, gl::GLenum::GL_REPEAT);

		glTexParameteri(gl::GLenum::GL_TEXTURE_2D, gl::GLenum::GL_TEXTURE_WRAP_T, gl::GLenum::GL_REPEAT);

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


	// load raymarching shader
	gl::GLuint raymarching_shader;
	{
		std::string vertex_code{};
		std::string fragment_code{};

		std::ifstream vertex_file{ "raymarch_vertex.glsl" };

		std::ifstream fragment_file{ "raymarch_fragment.frag" };

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
	framebuffer_t framebuffer{ screen_width, screen_height, 1, true };
	framebuffer_t framebuffer2{ screen_width, screen_height, 1, true };
	framebuffer_t framebuffer3{ screen_width, screen_height, 1, true };

	auto delta_time = 1.0F / 60.0F;
	auto now = std::chrono::high_resolution_clock::now();

	while (glfwWindowShouldClose(window) == 0)
	{
		glfwPollEvents();

		delta_time = std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock::now() - now).count();
		now = std::chrono::high_resolution_clock::now();
		cumulative_time += delta_time;

		process_input(delta_time);

		std::cout << "Delta time: " << delta_time << std::endl;

		// raymarching
		framebuffer2.bind();
		glClear(gl::ClearBufferMask::GL_COLOR_BUFFER_BIT | gl::ClearBufferMask::GL_DEPTH_BUFFER_BIT);
		gl::glUseProgram(raymarching_shader);
		glActiveTexture(gl::GLenum::GL_TEXTURE1);
		glBindTexture(gl::GLenum::GL_TEXTURE_3D, cloud_base_texture);
		glActiveTexture(gl::GLenum::GL_TEXTURE2);
		glBindTexture(gl::GLenum::GL_TEXTURE_3D, cloud_erosion_texture);
		glActiveTexture(gl::GLenum::GL_TEXTURE3);
		glBindTexture(gl::GLenum::GL_TEXTURE_2D, weather_map_texture);

		set_uniform(raymarching_shader, "cloud_base", 1);
		set_uniform(raymarching_shader, "cloud_erosion", 2);
		set_uniform(raymarching_shader, "weather_map", 3);
		set_uniform(raymarching_shader, "use_blue_noise", blue_noise);
		if (blue_noise)
		{
			glActiveTexture(gl::GLenum::GL_TEXTURE4);
			glBindTexture(gl::GLenum::GL_TEXTURE_2D, blue_noise_texture);
			set_uniform(raymarching_shader, "blue_noise", 4);
		}
		glActiveTexture(gl::GLenum::GL_TEXTURE5);
		glBindTexture(gl::GLenum::GL_TEXTURE_1D, mie_texture);
		set_uniform(raymarching_shader, "mie_texture", 5);

		set_uniform(raymarching_shader, "projection", camera.projection);
		set_uniform(raymarching_shader, "view", camera.transform.get_view_matrix());
		set_uniform(raymarching_shader, "camera_pos", glm::vec3{ camera.transform.position });
		set_uniform(raymarching_shader, "weather_map_visualization", weather_map);
		set_uniform(raymarching_shader, "low_frequency_noise_visualization",
			static_cast<int>(radio_button_value == 2));
		set_uniform(raymarching_shader, "high_frequency_noise_visualization",
			static_cast<int>(radio_button_value == 3));
		set_uniform(raymarching_shader, "multiple_scattering_approximation", multiple_scattering_approximation);
		set_uniform(raymarching_shader, "low_freq_noise_scale", low_freq_noise_scale);
		set_uniform(raymarching_shader, "weather_map_scale", weather_map_scale);
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
		set_uniform(raymarching_shader, "global_cloud_coverage", global_cloud_coverage);
		set_uniform(raymarching_shader, "anvil_bias", anvil_bias);
		wind_direction_normalized = normalize(wind_direction);
		set_uniform(raymarching_shader, "wind_direction", wind_direction_normalized);
		sun_direction_normalized = normalize(sun_direction);
		set_uniform(raymarching_shader, "sun_direction", sun_direction_normalized);
		set_uniform(raymarching_shader, "use_ambient", ambient);
		set_uniform(raymarching_shader, "turbidity", turbidity);

		static std::array<glm::vec3, 5> arr_up
		{
			normalize(glm::vec3{0, 1, 0}), normalize(glm::vec3{ 1, 0.01, 0 }),  glm::vec3{ -1, 0.01, 0 }, glm::vec3{ 0, 0.01, 1 }, glm::vec3{ 0, 0.01, -1 }
		};


		static std::array<glm::vec3, 5> arr_down
		{
			normalize(glm::vec3{0, -1, 0}), normalize(glm::vec3{ 1, -0.01, 0 }),  glm::vec3{ -1, -0.01, 0 }, glm::vec3{ 0, -0.01, 1 }, glm::vec3{ 0, -0.01, -1 }
		};

		// use average of 5 samples as ambient radiance
		// this could really be improved (and done on the GPU as well)
		static glm::vec3 ambient_radiance_up{};

		for (auto& el : arr_up)
		{
			ambient_radiance_up += 1000.0F*calculateSkyLuminanceRGB(-sun_direction_normalized, el, turbidity);
			std::cout << ambient_radiance_up.x << " " << ambient_radiance_up.y << " " << ambient_radiance_up.z << std::endl;

		}
		ambient_radiance_up /= 5.0F;


		std::cout << ambient_radiance_up.x << " " << ambient_radiance_up.y << " " << ambient_radiance_up.z << std::endl;

		set_uniform(raymarching_shader, "ambient_luminance_up", ambient_radiance_up);

		static glm::vec3 ambient_radiance_down{};

		for (auto& el : arr_down)
		{
			ambient_radiance_down += 1000.0F * calculateSkyLuminanceRGB(-sun_direction_normalized, el, turbidity);
			std::cout << ambient_radiance_down.x << " " << ambient_radiance_down.y << " " << ambient_radiance_down.z << std::endl;

		}
		ambient_radiance_down /= 5.0F;

		set_uniform(raymarching_shader, "ambient_luminance_down", ambient_radiance_down);

		glDrawElements(gl::GLenum::GL_TRIANGLES, 6, gl::GLenum::GL_UNSIGNED_INT, nullptr);

		if (blur)
		{
			// gaussian blur
			auto horizontal = true;
			auto amount = 4;
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
		}

		framebuffer_t::unbind();
		glClear(gl::ClearBufferMask::GL_COLOR_BUFFER_BIT | gl::ClearBufferMask::GL_DEPTH_BUFFER_BIT);
		gl::glUseProgram(tonemap_shader);

		framebuffer2.colour_attachments.front().bind(0);
		set_uniform(tonemap_shader, "full_screen", 0);
		set_uniform(tonemap_shader, "exposure_factor", exposure_factor);
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

			ImGui::RadioButton("low frequency noise", &radio_button_value, 2);
			ImGui::RadioButton("high frequency noise", &radio_button_value, 3);
			ImGui::NewLine();

			ImGui::Checkbox("use coverage from weather map", &weather_map);
			ImGui::NewLine();

			ImGui::Checkbox("blue noise jitter", &blue_noise);
			ImGui::NewLine();

			ImGui::Checkbox("gaussian blur", &blur);
			ImGui::NewLine();

			ImGui::SliderFloat("low frequency noise scale", &low_freq_noise_scale, 10.0F, 200000.0F, "%.5f");
			ImGui::NewLine();

			ImGui::SliderFloat("high frequency noise scale", &high_freq_noise_scale, 10.0F, 10000.0F, "%.5f");
			ImGui::NewLine();

			ImGui::SliderFloat("weather map scale", &weather_map_scale, 3000.0F, 300000.0F, "%.5f");
			ImGui::NewLine();

			ImGui::SliderFloat("high frequency noise factor", &high_freq_noise_factor, 0.0F, 1.0F, "%.5f");
			ImGui::NewLine();

			ImGui::SliderFloat("anvil bias", &anvil_bias, 0.0F, 1.0F, "%.5f");
			ImGui::NewLine();

			ImGui::SliderFloat("scattering factor", &scattering_factor, 0.01F, 1.0F, "%.5f");
			ImGui::NewLine();

			ImGui::SliderFloat("extinction factor", &extinction_factor, 0.01F, 1.0F, "%.5f");
			ImGui::NewLine();

			ImGui::SliderFloat3("wind direction", &wind_direction[0], -1.0F, 1.0F, "%.5f");
			ImGui::NewLine();

			ImGui::SliderFloat("sun direction x", &sun_direction[0], -1.0F, 1.0F, "%.5f");
			ImGui::NewLine();

			ImGui::SliderFloat("sun direction y", &sun_direction[1], -1.0F, 0.00001F, "%.5f");
			ImGui::NewLine();

			ImGui::SliderFloat("sun direction z", &sun_direction[2], -1.0F, 1.0F, "%.5f");
			ImGui::NewLine();

			ImGui::SliderFloat("cloud speed", &cloud_speed, 0.0F, 10.0F, "%.5f");
			ImGui::NewLine();

			ImGui::SliderFloat("exposure factor", &exposure_factor, 0.00001F, 0.0001F, "%.8f");
			ImGui::NewLine();

			ImGui::SliderFloat("global cloud coverage", &global_cloud_coverage, 0.0F, 1.0F, "%.5f");
			ImGui::NewLine();

			ImGui::SliderFloat("turbidity", &turbidity, 2.0F, 10.0F);
			ImGui::NewLine();

			ImGui::Checkbox("approximate ambient light", &ambient);
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

	const auto right = glm::vec3{ view_matrix[0][0], view_matrix[1][0], view_matrix[2][0] };
	const auto up = glm::vec3{ view_matrix[0][1], view_matrix[1][1], view_matrix[2][1] };
	const auto forward = -glm::vec3{ view_matrix[0][2], view_matrix[1][2], view_matrix[2][2] };


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
		x_offset = 0.0F;
	}

	if (y_offset != 0.0F)
	{
		camera.transform.rotation.x -= y_offset * camera.mouse_sensitivity;
		camera.transform.rotation.x = std::clamp(camera.transform.rotation.x, -89.0F, 89.0F);
		y_offset = 0.0F;
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
		x = static_cast<float>(x_pos);
		y = static_cast<float>(y_pos);
		first_mouse = false;
	}

	x_offset = static_cast<float>(x_pos) - x;
	y_offset = static_cast<float>(y_pos) - y;

	x = static_cast<float>(x_pos);
	y = static_cast<float>(y_pos);
}
