#define GLFW_INCLUDE_NONE

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string_view>

#include "camera.hpp"
#include "framebuffer.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "input.hpp"
#include "mesh.hpp"
#include "preetham.hpp"
#include "shader.hpp"
#include "stb_image.h"
#include "transforms.hpp"
#include "glbinding/glbinding.h"
#include "glbinding/gl/gl.h"
#include "GLFW/glfw3.h"


struct configuration_t
{
    std::string_view weather_map{};
    float            base_scale{};
    float            detail_scale{};
    float            weather_scale{};
    float            detail_factor{};
    glm::vec3        min{};
    glm::vec3        max{};
    float            a{};
    float            b{};
    float            c{};
    float            extinction{};
    float            scattering{};
    float            global_coverage{};
};

auto main() -> int
{
    constexpr auto screen_width  = 1280;
    constexpr auto screen_height = 720;

    const auto full_screen_quad_positions = std::vector{
        glm::vec3{-1.0F, -1.0F, 0.0F},
        glm::vec3{1.0F, -1.0F, 0.0F},
        glm::vec3{1.0F, 1.0F, 0.0F},
        glm::vec3{-1.0F, 1.0F, 0.0F}
    };

    const auto full_screen_quad_uvs = std::vector{
        glm::vec2{0.0F, 0.0F},
        glm::vec2{1.0F, 0.0F},
        glm::vec2{1.0F, 1.0F},
        glm::vec2{0.0F, 1.0F}
    };

    const auto full_screen_quad_indices = std::vector{0U, 1U, 2U, 2U, 3U, 0U};

    auto camera = camera_t{
        perspective(90.0F, static_cast<float>(screen_width) / screen_height, 0.01F, 100000.0F),
        {{0.0F, 10.0F, 0.0F, 1.0F}, {}, {1.0F, 1.0F, 1.0F}}
    };


    auto configurations = std::array{
        configuration_t
        {
            "perlin_test_cumulus",
            60000,
            1500,
            60000,
            0.5F,
            {-30000, 1000, -30000},
            {30000, 4000, 30000},
            0.55F,
            0.7F,
            0.00F,
            0.06F,
            0.06F,
            0.0F
        },
        configuration_t{
            "perlin_test_stratocumulus",
            40000,
            2000,
            60000,
            0.5F,
            {-30000, 1000, -30000},
            {30000, 4000, 30000},
            0.6F,
            0.75,
            0.0F,
            0.033F,
            0.033F,
            0.0F
        },
        configuration_t{
            "perlin_test_stratus",
            60000,
            3000,
            60000,
            0.33F,
            {-30000, 1000, -30000},
            {30000, 4000, 30000},
            0.6F,
            0.75F,
            0.0F,
            0.02F,
            0.02F,
            1.0F
        },
        configuration_t{
            "custom_cumulus",
            15000,
            1500,
            60000,
            0.5F,
            {-6000, 1000, -6000},
            {6000, 4000, 6000},
            0.6F,
            0.75F,
            0.0F,
            0.06F,
            0.06F,
            1.0F
        },
    };

    auto weather_maps = std::unordered_map<std::string_view, texture_t<2U>>{};

    const auto arr_up = std::array{
        normalize(glm::vec3{0, 1, 0}),
        normalize(glm::vec3{1, 0.01, 0}),
        glm::vec3{-1, 0.01, 0},
        glm::vec3{0, 0.01, 1},
        glm::vec3{0, 0.01, -1}
    };

    const auto arr_down = std::array{
        normalize(glm::vec3{0, -1, 0}),
        normalize(glm::vec3{1, -0.01, 0}),
        glm::vec3{-1, -0.01, 0},
        glm::vec3{0, -0.01, 1},
        glm::vec3{0, -0.01, -1}
    };

    auto radio_button_value{3};
    auto cfg_value{0};
    auto sun_intensity{1.0F};
    auto anvil_bias{0.0F};
    auto coverage_multiplier{1.0F};
    auto exposure_factor{0.000015F};
    auto turbidity{5.0F};
    auto multiple_scattering_approximation{true};
    auto blue_noise{false};
    auto blur{false};
    auto ambient{true};
    auto n{16};
    auto primary_ray_steps{64};
    auto secondary_ray_steps{16};
    auto cloud_speed{0.0F};
    auto density_multiplier{1.0F};
    auto wind_direction{glm::vec3{1.0F, 0.0F, 0.0F}};
    auto wind_direction_normalized{glm::vec3{}};
    auto sun_direction{glm::vec3{0.0F, -1.0F, 0.0F}};
    auto sun_direction_normalized{glm::vec3{}};

    // init glfw
    if (glfwInit() == 0) { std::exit(-1); }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, 0);

    const auto window = glfwCreateWindow(screen_width, screen_height, "clouds", nullptr, nullptr);
    if (window == nullptr) { std::exit(-1); }

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0); // disable v-sync
    glbinding::initialize(glfwGetProcAddress);
    gl::glViewport(0, 0, screen_width, screen_height);

    //init imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsLight();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    // load textures
    stbi_set_flip_vertically_on_load(1);
    const auto cloud_base_texture    = texture_t<3U>{128U, 128U, 128U, "textures/noise_shape.tga"};
    const auto cloud_erosion_texture = texture_t<3U>{64U, 64U, 64U, "textures/noise_erosion_hd.tga"};
    const auto mie_texture           = texture_t<1U>{
        1800U,
        0U,
        0U,
        "textures/mie_phase_function_normalized.hdr",
        gl::GLenum::GL_RGB32F,
        gl::GLenum::GL_RGB,
        gl::GLenum::GL_FLOAT
    };
    weather_maps["perlin_test_stratus"]       = texture_t<2U>{512U, 512U, 0U, "textures/perlin_test_stratus.tga"};
    weather_maps["perlin_test_stratocumulus"] = texture_t<2U>{512U, 512U, 0U, "textures/perlin_test_stratocumulus.tga"};
    weather_maps["perlin_test_cumulus"]       = texture_t<2U>{512U, 512U, 0U, "textures/perlin_test_cumulus.tga"};
    weather_maps["custom_cumulus"]            = texture_t<2U>{512U, 512U, 0U, "textures/custom_cumulus.tga"};
    const auto blue_noise_texture             = texture_t<2U>(512U, 512U, 0U, "textures/blue_noise.png");

    const auto quad               = mesh_t{full_screen_quad_positions, full_screen_quad_uvs, full_screen_quad_indices};
    const auto raymarching_shader = shader_t{"raymarch.vert", "raymarch.frag"};
    const auto blur_shader        = shader_t{"raymarch.vert", "blur.frag"};
    const auto tonemap_shader     = shader_t{"raymarch.vert", "tonemap.frag"};

    gl::glClearColor(0.0F, 0.0F, 0.0F, 1.0F);

    // create framebuffers
    auto framebuffer{framebuffer_t{screen_width, screen_height, 1, true}};
    auto framebuffer2{framebuffer_t{screen_width, screen_height, 1, true}};

    auto framebuffer3{framebuffer_t{screen_width, screen_height, 1, true}};

    auto delta_time = 1.0F / 60.0F;
    auto cumulative_time{0.0F};
    auto now = std::chrono::high_resolution_clock::now();

    while (glfwWindowShouldClose(window) == 0)
    {
        glfwPollEvents();

        delta_time = std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock::now() - now).count();
        now        = std::chrono::high_resolution_clock::now();
        cumulative_time += delta_time;

        process_input(delta_time, camera);

        std::cout << "Delta time: " << delta_time << std::endl;

        // raymarching
        auto& cfg = configurations[cfg_value];
        framebuffer2.bind();
        glClear(gl::ClearBufferMask::GL_COLOR_BUFFER_BIT | gl::ClearBufferMask::GL_DEPTH_BUFFER_BIT);
        raymarching_shader.use();
        cloud_base_texture.bind(1);
        cloud_erosion_texture.bind(2);
        weather_maps[cfg.weather_map].bind(3);

        raymarching_shader.set_uniform("cloud_base", 1);
        raymarching_shader.set_uniform("cloud_erosion", 2);
        raymarching_shader.set_uniform("weather_map", 3);
        raymarching_shader.set_uniform("use_blue_noise", blue_noise);

        if (blue_noise)
        {
            blue_noise_texture.bind(4);
            raymarching_shader.set_uniform("blue_noise", 4);
        }
        mie_texture.bind(5);
        raymarching_shader.set_uniform("mie_texture", 5);

        raymarching_shader.set_uniform("projection", camera.projection);
        raymarching_shader.set_uniform("view", camera.transform.get_view_matrix());
        raymarching_shader.set_uniform("camera_pos", glm::vec3{camera.transform.position});
        raymarching_shader.set_uniform("low_frequency_noise_visualization", static_cast<int>(radio_button_value == 2));
        raymarching_shader.set_uniform("high_frequency_noise_visualization", static_cast<int>(radio_button_value == 3));
        raymarching_shader.set_uniform("multiple_scattering_approximation", multiple_scattering_approximation);
        raymarching_shader.set_uniform("low_freq_noise_scale", cfg.base_scale);
        raymarching_shader.set_uniform("weather_map_scale", cfg.weather_scale);
        raymarching_shader.set_uniform("high_freq_noise_scale", cfg.detail_scale);
        raymarching_shader.set_uniform("scattering_factor", cfg.scattering);
        raymarching_shader.set_uniform("extinction_factor", cfg.extinction);
        raymarching_shader.set_uniform("sun_intensity", sun_intensity);
        raymarching_shader.set_uniform("high_freq_noise_factor", cfg.detail_factor);
        raymarching_shader.set_uniform("N", n);
        raymarching_shader.set_uniform("a", cfg.a);
        raymarching_shader.set_uniform("b", cfg.b);
        raymarching_shader.set_uniform("c", cfg.c);
        raymarching_shader.set_uniform("primary_ray_steps", primary_ray_steps);
        raymarching_shader.set_uniform("secondary_ray_steps", secondary_ray_steps);
        raymarching_shader.set_uniform("time", cumulative_time);
        raymarching_shader.set_uniform("cloud_speed", cloud_speed);
        raymarching_shader.set_uniform("global_cloud_coverage", cfg.global_coverage);
        raymarching_shader.set_uniform("anvil_bias", anvil_bias);
        wind_direction_normalized = normalize(wind_direction);
        raymarching_shader.set_uniform("wind_direction", wind_direction_normalized);
        sun_direction_normalized = normalize(sun_direction);
        raymarching_shader.set_uniform("sun_direction", sun_direction_normalized);
        raymarching_shader.set_uniform("use_ambient", ambient);
        raymarching_shader.set_uniform("turbidity", turbidity);
        raymarching_shader.set_uniform("coverage_mult", coverage_multiplier);
        raymarching_shader.set_uniform("density_mult", density_multiplier);
        raymarching_shader.set_uniform("aabb_max", cfg.max);
        raymarching_shader.set_uniform("aabb_min", cfg.min);

        // use average of 5 samples as ambient radiance
        // this could really be improved (and done on the GPU as well)
        auto ambient_luminance_up{glm::vec3{}};
        for (const auto& el : arr_up)
        {
            ambient_luminance_up += 1000.0F * calculate_sky_luminance_RGB(-sun_direction_normalized, el, turbidity);
        }
        ambient_luminance_up /= 5.0F;
        raymarching_shader.set_uniform("ambient_luminance_up", ambient_luminance_up);

        auto ambient_luminance_down{glm::vec3{}};
        for (const auto& el : arr_down)
        {
            ambient_luminance_down += 1000.0F * calculate_sky_luminance_RGB(-sun_direction_normalized, el, turbidity);
        }
        ambient_luminance_down /= 5.0F;
        raymarching_shader.set_uniform("ambient_luminance_down", ambient_luminance_down);

        quad.draw();

        if (blur)
        {
            // gaussian blur
            auto horizontal = true;
            auto amount     = 4;
            blur_shader.use();
            blur_shader.set_uniform("full_screen", 0);
            for (auto i = 0; i < amount; i++)
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
                blur_shader.set_uniform("horizontal", horizontal);

                quad.draw();
                horizontal = !horizontal;
            }
        }

        framebuffer_t::unbind();
        glClear(gl::ClearBufferMask::GL_COLOR_BUFFER_BIT | gl::ClearBufferMask::GL_DEPTH_BUFFER_BIT);
        tonemap_shader.use();

        framebuffer2.colour_attachments.front().bind(0);
        tonemap_shader.set_uniform("full_screen", 0);
        tonemap_shader.set_uniform("exposure_factor", exposure_factor);
        quad.draw();

        // render gui
        if (options())
        {
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

            ImGui::Begin("options");

            ImGui::SliderInt("number of primary ray steps", &primary_ray_steps, 1, 500, "%d");
            ImGui::SliderInt("number of secondary ray steps", &secondary_ray_steps, 1, 100, "%d");
            ImGui::NewLine();

            ImGui::RadioButton("low frequency noise", &radio_button_value, 2);
            ImGui::RadioButton("high frequency noise", &radio_button_value, 3);
            ImGui::NewLine();

            ImGui::RadioButton("cumulus map", &cfg_value, 0);
            ImGui::RadioButton("stratocumulus map", &cfg_value, 1);
            ImGui::RadioButton("stratus map", &cfg_value, 2);
            ImGui::RadioButton("one cumulus map", &cfg_value, 3);
            ImGui::NewLine();

            ImGui::Checkbox("blue noise jitter", &blue_noise);
            ImGui::Checkbox("gaussian blur", &blur);
            ImGui::NewLine();

            ImGui::SliderFloat("low frequency noise scale", &cfg.base_scale, 10.0F, 200000.0F, "%.5f");
            ImGui::SliderFloat("high frequency noise scale", &cfg.detail_scale, 10.0F, 10000.0F, "%.5f");
            ImGui::SliderFloat("weather map scale", &cfg.weather_scale, 3000.0F, 300000.0F, "%.5f");
            ImGui::NewLine();

            ImGui::SliderFloat("high frequency noise factor", &cfg.detail_factor, 0.0F, 1.0F, "%.5f");
            ImGui::SliderFloat("anvil bias", &anvil_bias, 0.0F, 1.0F, "%.5f");
            ImGui::SliderFloat("scattering factor", &cfg.scattering, 0.01F, 1.0F, "%.5f");
            ImGui::SliderFloat("extinction factor", &cfg.extinction, 0.01F, 1.0F, "%.5f");
            ImGui::SliderFloat3("wind direction", &wind_direction[0], -1.0F, 1.0F, "%.5f");
            ImGui::SliderFloat3("aabb max", &cfg.max[0], -30000.0F, 30000.0F, "%.5f");
            ImGui::SliderFloat3("aabb min", &cfg.min[0], -30000.0F, 30000.0F, "%.5f");
            ImGui::SliderFloat("sun direction x", &sun_direction[0], -1.0F, 1.0F, "%.5f");
            ImGui::SliderFloat("sun direction y", &sun_direction[1], -1.0F, -0.001F, "%.5f");
            ImGui::SliderFloat("sun direction z", &sun_direction[2], -1.0F, 1.0F, "%.5f");
            ImGui::SliderFloat("cloud speed", &cloud_speed, 0.0F, 10.0F, "%.5f");
            ImGui::SliderFloat("exposure factor", &exposure_factor, 0.00001F, 0.0001F, "%.8f");
            ImGui::SliderFloat("global cloud coverage", &cfg.global_coverage, 0.0F, 1.0F, "%.5f");
            ImGui::SliderFloat("turbidity", &turbidity, 2.0F, 20.0F);
            ImGui::SliderFloat("coverage_mult", &coverage_multiplier, 0.0F, 1.0F);
            ImGui::SliderFloat("density_mult", &density_multiplier, 0.0F, 5.0F);
            ImGui::NewLine();

            ImGui::Checkbox("approximate ambient light", &ambient);
            ImGui::Checkbox("multiple scattering approximation", &multiple_scattering_approximation);
            ImGui::SliderInt("octaves count", &n, 1, 16, "%d");
            ImGui::SliderFloat("attenuation", &cfg.a, 0.01F, 1.0F, "%.2f");
            ImGui::SliderFloat("contribution", &cfg.b, 0.01F, 1.0F, "%.2f");
            ImGui::SliderFloat("eccentricity attenuation", &cfg.c, 0.01F, 1.0F, "%.2f");
            ImGui::End();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();
}
