#include "noise.hpp"
#include "stb_image_write.h"

#include <algorithm>
#include <fstream>

auto remap(float value, float min, float max, float new_min, float new_max) noexcept -> float
{
    return new_min + (value - min) / (max - min) * (new_max - new_min);
}

auto generate_cloud_shape_textures() noexcept -> void
{
    // frequency multiplication. No boundary check etc. but fine for this small tool.
    const float frequency_mul[6] = {2.0F, 8.0F, 14.0F, 20.0F, 26.0F, 32.0F};
    // special weight for perlin-worley

    // cloud base shape
    // note: all channels could be combined once here to reduce memory bandwidth requirements.
    auto cloud_base_shape_texture_size  = 128;
    int  cloud_base_shape_row_bytes     = cloud_base_shape_texture_size * sizeof(unsigned char) * 4;
    auto cloud_base_shape_slice_bytes   = cloud_base_shape_row_bytes * cloud_base_shape_texture_size;
    auto cloud_base_shape_volume_bytes  = cloud_base_shape_slice_bytes * cloud_base_shape_texture_size;
    auto cloud_base_shape_texels        = static_cast<unsigned char *>(malloc(cloud_base_shape_volume_bytes));
    auto cloud_base_shape_texels_packed = static_cast<unsigned char *>(malloc(cloud_base_shape_volume_bytes));

    for (auto s = 0; s < cloud_base_shape_texture_size; s++) {
        const auto norm_fact = glm::vec3(1.0F / static_cast<float>(cloud_base_shape_texture_size));
        for (auto t = 0; t < cloud_base_shape_texture_size; t++) {
            for (auto r = 0; r < cloud_base_shape_texture_size; r++) {
                auto coord = glm::vec3(s, t, r) * norm_fact;

                // perlin fBm
                const auto octave_count = 3;
                const auto frequency    = 8.0F;
                auto       perlin_noise = perlin(coord, frequency, octave_count);

                auto perlin_worley_noise = 0.0F;
                {
                    const float cell_count    = 4;
                    const auto  worley_noise0 = worley(coord, cell_count * frequency_mul[0]);
                    const auto  worley_noise1 = worley(coord, cell_count * frequency_mul[1]);
                    const auto  worley_noise2 = worley(coord, cell_count * frequency_mul[2]);
                    const auto  worley_noise3 = 1.0F - worley(coord, cell_count * frequency_mul[3]);
                    const auto  worley_noise4 = 1.0F - worley(coord, cell_count * frequency_mul[4]);
                    const auto  worley_noise5 = 1.0F - worley(coord, cell_count * frequency_mul[5]);
                    // half the frequency of a texel, we should not go further (with cellCount = 32 and texture size = 64)

                    auto worley_fbm = worley_noise0 * 0.625F + worley_noise1 * 0.25F + worley_noise2 * 0.125F;

                    // perlin-worley is based on description in GPU Pro 7: Real Time Volumetric Cloudscapes
                    // however it is not clear the text and the image are matching
                    // images does not seem to match what the result  from the description in text would give
                    // also there are a lot of fudge factors in the code
                    // perlin_worley_noise = remap(worley_fbm, 0.0, 1.0, 0.0, perlin_noise);
                    // matches figure 4.7 better
                    perlin_worley_noise = std::clamp(remap(1.0F - perlin_noise, 0.0F, 1.0F, -(1.0F - worley_fbm), 1.0F),
                                                     0.0F, 1.0F);
                    // mapping perlin noise in between worley as minimum and 1.0 as maximum (as described in text of p.101 of GPU Pro 7)
                }

                const float cell_count    = 4;
                auto        worley_noise0 = worley(coord, cell_count * 1);
                auto        worley_noise1 = worley(coord, cell_count * 2);
                auto        worley_noise2 = worley(coord, cell_count * 4);
                auto        worley_noise3 = worley(coord, cell_count * 8);
                auto        worley_noise4 = worley(coord, cell_count * 16);

                // three different worley fbms
                auto worley_fbm0 = worley_noise1 * 0.625F + worley_noise2 * 0.25F + worley_noise3 * 0.125F;
                auto worley_fbm1 = worley_noise2 * 0.625F + worley_noise3 * 0.25F + worley_noise4 * 0.125F;
                auto worley_fbm2 = worley_noise3 * 0.75F + worley_noise4 * 0.25F;
                // cell_count = 4 -> worley_noise5 is just noise due to sampling frequency = texel frequency so only take into account 2 frequencies for fBm

                auto addr = r * cloud_base_shape_texture_size * cloud_base_shape_texture_size + t * cloud_base_shape_texture_size + s;

                addr *= 4;
                cloud_base_shape_texels[addr]     = static_cast<unsigned char>(255.0f * perlin_worley_noise);
                cloud_base_shape_texels[addr + 1] = static_cast<unsigned char>(255.0f * (1.0F - worley_fbm0));
                cloud_base_shape_texels[addr + 2] = static_cast<unsigned char>(255.0f * (1.0F - worley_fbm1));
                cloud_base_shape_texels[addr + 3] = static_cast<unsigned char>(255.0f * (1.0F - worley_fbm2));

                float value = 0.0;
                {
                    // pack the channels for direct usage in shader
                    auto low_freq_fbm = worley_fbm0 * 0.625F + worley_fbm1 * 0.25F + worley_fbm2 * 0.125F;
                    auto base_cloud   = perlin_worley_noise;
                    value             = std::clamp(remap(base_cloud, -(1.0F - low_freq_fbm), 1.0F, 0.0F, 1.0F), 0.0F,
                                       1.0F);
                }

                cloud_base_shape_texels_packed[addr]     = static_cast<unsigned char>(255.0f * value);
                cloud_base_shape_texels_packed[addr + 1] = static_cast<unsigned char>(255.0f * value);
                cloud_base_shape_texels_packed[addr + 2] = static_cast<unsigned char>(255.0f * value);
                cloud_base_shape_texels_packed[addr + 3] = static_cast<unsigned char>(255.0f);
            }
        }
    }

    {
        auto width  = cloud_base_shape_texture_size * cloud_base_shape_texture_size;
        auto height = cloud_base_shape_texture_size;
        stbi_write_tga("noise_shape_test.tga", width, height, 4, cloud_base_shape_texels);
        stbi_write_tga("noise_shape_test_packed.tga", width, height, 4, cloud_base_shape_texels_packed);
    }

    // cloud detail texture
    // Note: all channels could be combined once here to reduce memory bandwidth requirements.
    auto cloud_erosion_texture_size  = 32;
    int  cloud_erosion_row_bytes     = cloud_erosion_texture_size * sizeof(unsigned char) * 4;
    auto cloud_erosion_slice_bytes   = cloud_erosion_row_bytes * cloud_erosion_texture_size;
    auto cloud_erosion_volume_bytes  = cloud_erosion_slice_bytes * cloud_erosion_texture_size;
    auto cloud_erosion_texels        = static_cast<unsigned char *>(malloc(cloud_erosion_volume_bytes));
    auto cloud_erosion_texels_packed = static_cast<unsigned char *>(malloc(cloud_erosion_volume_bytes));

    for (auto s = 0; s < cloud_erosion_texture_size; s++) {
        const auto norm_fact = glm::vec3(1.0F / static_cast<float>(cloud_erosion_texture_size));
        for (auto t = 0; t < cloud_erosion_texture_size; t++) {
            for (auto r = 0; r < cloud_erosion_texture_size; r++) {
                auto coord = glm::vec3(s, t, r) * norm_fact;

                // 3 octaves
                const float cell_count    = 2;
                auto        worley_noise0 = worley(coord, cell_count * 1);
                auto        worley_noise1 = worley(coord, cell_count * 2);
                auto        worley_noise2 = worley(coord, cell_count * 4);
                auto        worley_noise3 = worley(coord, cell_count * 8);
                auto        worley_fbm0   = worley_noise0 * 0.625F + worley_noise1 * 0.25F + worley_noise2 * 0.125F;
                auto        worley_fbm1   = worley_noise1 * 0.625F + worley_noise2 * 0.25F + worley_noise3 * 0.125F;
                auto        worley_fbm2   = worley_noise2 * 0.75F + worley_noise3 * 0.25F;
                // cell_count = 4 -> worley_noise4 is just noise due to sampling frequency = texel frequency so only take into account 2 frequencies for fBm

                auto addr = r * cloud_erosion_texture_size * cloud_erosion_texture_size + t * cloud_erosion_texture_size
                            + s;
                addr *= 4;
                cloud_erosion_texels[addr]     = static_cast<unsigned char>(255.0f * (1 - worley_fbm0));
                cloud_erosion_texels[addr + 1] = static_cast<unsigned char>(255.0f * (1 - worley_fbm1));
                cloud_erosion_texels[addr + 2] = static_cast<unsigned char>(255.0f * (1 - worley_fbm2));
                cloud_erosion_texels[addr + 3] = static_cast<unsigned char>(255.0f);

                float value = 0.0;
                {
                    value = worley_fbm0 * 0.625F + worley_fbm1 * 0.25F + worley_fbm2 * 0.125F;
                }
                value                                 = 1.0F - value;
                cloud_erosion_texels_packed[addr]     = static_cast<unsigned char>(255.0f * value);
                cloud_erosion_texels_packed[addr + 1] = static_cast<unsigned char>(255.0f * value);
                cloud_erosion_texels_packed[addr + 2] = static_cast<unsigned char>(255.0f * value);
                cloud_erosion_texels_packed[addr + 3] = static_cast<unsigned char>(255.0f);
            }
        }
    }

    {
        auto width  = cloud_erosion_texture_size * cloud_erosion_texture_size;
        auto height = cloud_erosion_texture_size;
        stbi_write_tga("noise_erosion_test.tga", width, height, 4, cloud_erosion_texels);
        stbi_write_tga("noise_erosion_test_packed.tga", width, height, 4, cloud_erosion_texels_packed);
    }

    free(cloud_base_shape_texels);
    free(cloud_base_shape_texels_packed);
    free(cloud_erosion_texels);
    free(cloud_erosion_texels_packed);
}

auto generate_perlin_noise() noexcept -> void
{
    const auto cloud_base_shape_texture_size  = 512;
    const int  cloud_base_shape_row_bytes     = cloud_base_shape_texture_size * sizeof(unsigned char) * 4;
    const auto cloud_base_shape_slice_bytes   = cloud_base_shape_row_bytes * cloud_base_shape_texture_size;
    const auto cloud_base_shape_volume_bytes  = cloud_base_shape_slice_bytes * cloud_base_shape_texture_size;
    const auto cloud_base_shape_texels        = static_cast<unsigned char *>(malloc(cloud_base_shape_volume_bytes));
    const auto cloud_base_shape_texels_packed = static_cast<unsigned char *>(malloc(cloud_base_shape_volume_bytes));

    for (auto s = 0; s < cloud_base_shape_texture_size; s++) {
        const auto norm_fact = glm::vec3(1.0F / static_cast<float>(cloud_base_shape_texture_size));
        for (auto t = 0; t < cloud_base_shape_texture_size; t++) {
            const auto coord = glm::vec3(s, t, 0) * norm_fact;

            // perlin fBm
            const auto octave_count  = 5;
            const auto frequency     = 4.0F;
            const auto perlin_noiseb = std::clamp(pow(perlin(coord, frequency, octave_count), 1.0F), 0.0F, 1.0F);
            const auto perlin_noiseg = std::clamp(pow(perlin(coord, frequency, octave_count), 2.0F), 0.0F, 1.0F);
            const auto perlin_noiser = std::clamp(pow(perlin(coord, frequency, octave_count), 4.0F), 0.0F, 1.0F);

            const auto perlin_noisea = std::clamp(1.5F * pow(perlin(coord, frequency, 1), 2.0F), 0.0F, 1.0F);

            auto perlin_worley_noise_g = 0.0F;
            auto perlin_worley_noise_b = 0.0F;
            {
                const float cell_count    = 4;
                const auto  worley_noise0 = pow(worley(coord, cell_count * 2), 2);
                const auto  worley_noise1 = pow(worley(coord, cell_count * 4), 2);
                const auto  worley_noise2 = pow(worley(coord, cell_count * 8), 2);
                const auto  worley_noise3 = pow(worley(coord, cell_count * 16), 2);
                const auto  worley_noise4 = pow(worley(coord, cell_count * 32), 2);
                // half the frequency of a texel, we should not go further (with cellCount = 32 and texture size = 64)

                auto worley_fbm = worley_noise0 * 0.5F + worley_noise1 * 0.25F + worley_noise2 * 0.125F + worley_noise3 * 0.075F + worley_noise4 * 0.05F;
                //worley_fbm = worley_noise0;

                // perlin-worley is based on description in GPU Pro 7: Real Time Volumetric Cloudscapes
                // however it is not clear the text and the image are matching
                // images does not seem to match what the result  from the description in text would give
                // also there are a lot of fudge factors in the code
                //
                // matches figure 4.7 better
                // mapping perlin noise in between worley as minimum and 1.0 as maximum (as described in text of p.101 of GPU Pro 7)
                auto perlin_worley_noise_r = std::clamp(remap(1.0F - worley_fbm, 1.0F - perlin_noiser, 1.0, 0.0, 1.0), 0.0F,
                                                        1.0F);
                perlin_worley_noise_g      = std::clamp(remap(1.0F - worley_fbm, 1.0F - perlin_noiseg, 1.0, 0.0, 1.0), 0.0F,
                                                   1.0F);
                perlin_worley_noise_b      = std::clamp(remap(1.0F - worley_fbm, 1.0F - perlin_noiseb, 1.0, 0.0, 1.0), 0.0F,
                                                   1.0F);
            }

            auto addr = t * cloud_base_shape_texture_size + s;

            addr *= 4;
            cloud_base_shape_texels[addr]     = static_cast<unsigned char>(255.0f * perlin_worley_noise_g);
            cloud_base_shape_texels[addr + 1] = static_cast<unsigned char>(255.0f * perlin_worley_noise_b);
            cloud_base_shape_texels[addr + 2] = static_cast<unsigned char>(255.0f * perlin_noisea);
            cloud_base_shape_texels[addr + 3] = static_cast<unsigned char>(255.0F);
        }
    }

    {
        const auto width  = cloud_base_shape_texture_size;
        const auto height = cloud_base_shape_texture_size;
        stbi_write_tga("weather_map.tga", width, height, 4, cloud_base_shape_texels);
    }

    free(cloud_base_shape_texels);
    free(cloud_base_shape_texels_packed);
}

auto generate_worley_noise() noexcept -> void
{
    const auto cloud_base_shape_texture_size  = 512;
    const int  cloud_base_shape_row_bytes     = cloud_base_shape_texture_size * sizeof(unsigned char) * 4;
    const auto cloud_base_shape_slice_bytes   = cloud_base_shape_row_bytes * cloud_base_shape_texture_size;
    const auto cloud_base_shape_volume_bytes  = cloud_base_shape_slice_bytes * cloud_base_shape_texture_size;
    const auto cloud_base_shape_texels        = static_cast<unsigned char *>(malloc(cloud_base_shape_volume_bytes));
    const auto cloud_base_shape_texels_packed = static_cast<unsigned char *>(malloc(cloud_base_shape_volume_bytes));

    for (auto s = 0; s < cloud_base_shape_texture_size; s++) {
        const auto norm_fact = glm::vec3(1.0F / static_cast<float>(cloud_base_shape_texture_size));
        for (auto t = 0; t < cloud_base_shape_texture_size; t++) {
            const auto coord = glm::vec3(s, t, 0) * norm_fact;

            // perlin fBm
            const auto octave_count  = 5;
            const auto frequency     = 8.0F;
            const auto perlin_noiser = std::clamp(1.25F * perlin(coord, frequency, octave_count), 0.0F, 1.0F);
            auto       perlin_noiseb = std::clamp(1.5F * perlin(coord, 4.0F, 1), 0.0F, 1.0F);

            const float cell_count    = 4;
            const auto  worley_noise0 = worley(coord, cell_count * 2);
            const auto  worley_noise1 = worley(coord, cell_count * 4);
            const auto  worley_noise2 = worley(coord, cell_count * 8);

            const auto worley_fbm = worley_noise0 * 0.625F + worley_noise1 * 0.25F + worley_noise2 * 0.125F;

            // perlin-worley is based on description in GPU Pro 7: Real Time Volumetric Cloudscapes
            // however it is not clear the text and the image are matching
            // images does not seem to match what the result  from the description in text would give
            // also there are a lot of fudge factors in the code
            // perlin_worley_noise = remap(worley_fbm, 0.0, 1.0, 0.0, perlin_noise);
            // matches figure 4.7 better
            auto perlin_worley_noise = remap(perlin_noiser, 0.0F, 1.0F, worley_fbm, 1.0F);
            // mapping perlin noise in between worley as minimum and 1.0 as maximum (as described in text of p.101 of GPU Pro 7)

            auto addr = t * cloud_base_shape_texture_size + s;

            addr *= 4;
            cloud_base_shape_texels[addr]     = static_cast<unsigned char>(255.0f * worley_fbm);
            cloud_base_shape_texels[addr + 1] = static_cast<unsigned char>(255.0f * worley_fbm);
            cloud_base_shape_texels[addr + 2] = static_cast<unsigned char>(255.0f * worley_fbm);
            cloud_base_shape_texels[addr + 3] = static_cast<unsigned char>(255.0f);
        }
    }

    {
        const auto width  = cloud_base_shape_texture_size;
        const auto height = cloud_base_shape_texture_size;
        stbi_write_tga("worley_fbm.tga", width, height, 4, cloud_base_shape_texels);
    }

    free(cloud_base_shape_texels);
    free(cloud_base_shape_texels_packed);
}

// generates cloud shape and erosion textures, both packed and unpacked
auto main() noexcept -> int
{
    stbi_flip_vertically_on_write(1);

    generate_cloud_shape_textures();
}
