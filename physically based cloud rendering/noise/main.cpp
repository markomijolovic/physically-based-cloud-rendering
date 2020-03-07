#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "noise.hpp"

float remap(float value, float min, float max, float new_min, float new_max)
{
	return new_min + (value - min) / (max - min) * (new_max - new_min);
}

int main()
{
	stbi__flip_vertically_on_write = true;
	
	//
	// Generate cloud shape and erosion texture similarly GPU Pro 7 chapter II-4
	//

	// Frequence multiplicator. No boudary check etc. but fine for this small tool.
	const float frequence_mul[6] = {2.0F, 8.0F, 14.0F, 20.0F, 26.0F, 32.0F}; // special weight for perling worley

	// Cloud base shape (will be used to generate PerlingWorley noise in he shader)
	// Note: all channels could be combined once here to reduce memory bandwith requirements.
	auto cloud_base_shape_texture_size = 128;
	// !!! If this is reduce, you hsould also reduce the number of frequency in the fmb noise  !!!
	int  cloud_base_shape_row_bytes     = cloud_base_shape_texture_size * sizeof(unsigned char) * 4;
	auto cloud_base_shape_slice_bytes   = cloud_base_shape_row_bytes * cloud_base_shape_texture_size;
	auto cloud_base_shape_volume_bytes  = cloud_base_shape_slice_bytes * cloud_base_shape_texture_size;
	auto cloud_base_shape_texels       = static_cast<unsigned char*>(malloc(cloud_base_shape_volume_bytes));
	auto cloud_base_shape_texels_packed = static_cast<unsigned char*>(malloc(cloud_base_shape_volume_bytes));
	for (auto s = 0; s < cloud_base_shape_texture_size; s++) //for (int s = 0; s<gCloudBaseShapeTextureSize; s++)
	{
		const auto norm_fact = glm::vec3(1.0F / float(cloud_base_shape_texture_size));
		for (auto t = 0; t < cloud_base_shape_texture_size; t++)
		{
			for (auto r = 0; r < cloud_base_shape_texture_size; r++)
			{
				auto coord = glm::vec3(s, t, r) * norm_fact;

				// Perlin FBM noise
				const auto octaveCount = 3;
				const auto frequency   = 8.0F;
				auto       perlinNoise = perlin(coord, frequency, octaveCount);

				auto perlin_worley_noise = 0.0F;
				{
					const float cell_count    = 4;
					const auto  worley_noise0 = 1.0F - worley(coord, cell_count * frequence_mul[0]);
					const auto  worley_noise1 = 1.0F - worley(coord, cell_count * frequence_mul[1]);
					const auto  worley_noise2 = 1.0F - worley(coord, cell_count * frequence_mul[2]);
					const auto  worley_noise3 = 1.0F - worley(coord, cell_count * frequence_mul[3]);
					const auto  worley_noise4 = 1.0F - worley(coord, cell_count * frequence_mul[4]);
					const auto  worley_noise5 = 1.0F - worley(coord, cell_count * frequence_mul[5]);
					// half the frequency of texel, we should not go further (with cellCount = 32 and texture size = 64)

					auto worley_fbm = worley_noise0 * 0.625F + worley_noise1 * 0.25F + worley_noise2 * 0.125F;

					// Perlin Worley is based on description in GPU Pro 7: Real Time Volumetric Cloudscapes.
					// However it is not clear the text and the image are matching: images does not seem to match what the result  from the description in text would give.
					// Also there are a lot of fudge factor in the code, e.g. *0.2, so it is really up to you to fine the formula you like.
					//PerlinWorleyNoise = remap(worleyFBM, 0.0, 1.0, 0.0, perlinNoise);	// Matches better what figure 4.7 (not the following up text description p.101). Maps worley between newMin as 0 and 
					perlin_worley_noise = remap(perlinNoise, 0.0F, 1.0F, worley_fbm, 1.0F);
					// mapping perlin noise in between worley as minimum and 1.0 as maximum (as described in text of p.101 of GPU Pro 7) 
				}

				const float cell_count    = 4;
				auto        worley_noise0 = 1.0F - worley(coord, cell_count * 1);
				auto        worley_noise1 = 1.0F - worley(coord, cell_count * 2);
				auto        worley_noise2 = 1.0F - worley(coord, cell_count * 4);
				auto        worley_noise3 = 1.0F - worley(coord, cell_count * 8);
				auto        worley_noise4 = 1.0F - worley(coord, cell_count * 16);
				//float worleyNoise5 = (1.0f - Tileable3dNoise::WorleyNoise(coord, cellCount * 32));	//cellCount=2 -> half the frequency of texel, we should not go further (with cellCount = 32 and texture size = 64)

				// Three frequency of Worley FBM noise
				auto worley_fbm0 = worley_noise1 * 0.625F + worley_noise2 * 0.25F + worley_noise3 * 0.125F;
				auto worley_fbm1 = worley_noise2 * 0.625F + worley_noise3 * 0.25F + worley_noise4 * 0.125F;
				//float worleyFBM2 = worleyNoise3*0.625f + worleyNoise4*0.25f + worleyNoise5*0.125f;
				auto worley_fbm2 = worley_noise3 * 0.75F + worley_noise4 * 0.25F;
				// cellCount=4 -> worleyNoise5 is just noise due to sampling frequency=texel frequency. So only take into account 2 frequencies for FBM

				auto addr = r * cloud_base_shape_texture_size * cloud_base_shape_texture_size + t * cloud_base_shape_texture_size +
				            s;

				addr *= 4;
				cloud_base_shape_texels[addr]     = unsigned char(255.0f * perlin_worley_noise);
				cloud_base_shape_texels[addr + 1] = unsigned char(255.0f * worley_fbm0);
				cloud_base_shape_texels[addr + 2] = unsigned char(255.0f * worley_fbm1);
				cloud_base_shape_texels[addr + 3] = unsigned char(255.0f * worley_fbm2);

				float value = 0.0;
				{
					// pack the channels for direct usage in shader
					auto lowFreqFBM = worley_fbm0 * 0.625F + worley_fbm1 * 0.25F + worley_fbm2 * 0.125F;
					auto baseCloud  = perlin_worley_noise;
					value           = remap(baseCloud, -(1.0F - lowFreqFBM), 1.0F, 0.0F, 1.0F);
					// Saturate
					value = std::fminf(value, 1.0F);
					value = std::fmaxf(value, 0.0F);
				}
				cloud_base_shape_texels_packed[addr]     = unsigned char(255.0f * value);
				cloud_base_shape_texels_packed[addr + 1] = unsigned char(255.0f * value);
				cloud_base_shape_texels_packed[addr + 2] = unsigned char(255.0f * value);
				cloud_base_shape_texels_packed[addr + 3] = unsigned char(255.0f);
			}
		}
	}
	// end parallel_for
	{
		auto width  = cloud_base_shape_texture_size * cloud_base_shape_texture_size;
		auto height = cloud_base_shape_texture_size;
		stbi_write_tga("noiseShape.tga", width, height, 4, cloud_base_shape_texels);
		stbi_write_tga("noiseShapePacked.tga", width, height, 4, cloud_base_shape_texels_packed);
		//writeTGA("noiseShape.tga", width, height, cloudBaseShapeTexels);
		//writeTGA("noiseShapePacked.tga", width, height, cloudBaseShapeTexelsPacked);
	}


	// Detail texture behing different frequency of Worley noise
	// Note: all channels could be combined once here to reduce memory bandwith requirements.
	auto cloud_erosion_texture_size  = 32;
	int  cloud_erosion_row_bytes     = cloud_erosion_texture_size * sizeof(unsigned char) * 4;
	auto cloud_erosion_slice_bytes   = cloud_erosion_row_bytes * cloud_erosion_texture_size;
	auto cloud_erosion_volume_bytes  = cloud_erosion_slice_bytes * cloud_erosion_texture_size;
	auto cloud_erosion_texels       = static_cast<unsigned char*>(malloc(cloud_erosion_volume_bytes));
	auto cloud_erosion_texels_packed = static_cast<unsigned char*>(malloc(cloud_erosion_volume_bytes));
	for (auto s = 0; s < cloud_erosion_texture_size; s++) //for (int s = 0; s<gCloudErosionTextureSize; s++)
	{
		const auto normFact = glm::vec3(1.0F / float(cloud_erosion_texture_size));
		for (auto t = 0; t < cloud_erosion_texture_size; t++)
		{
			for (auto r = 0; r < cloud_erosion_texture_size; r++)
			{
				auto coord = glm::vec3(s, t, r) * normFact;

#if 1
				// 3 octaves
				const float cellCount    = 2;
				auto        worley_noise0 = 1.0F - worley(coord, cellCount * 1);
				auto        worley_noise1 = 1.0F - worley(coord, cellCount * 2);
				auto        worley_noise2 = 1.0F - worley(coord, cellCount * 4);
				auto        worley_noise3 = 1.0F - worley(coord, cellCount * 8);
				auto        worley_fbm0   = worley_noise0 * 0.625F + worley_noise1 * 0.25F + worley_noise2 * 0.125F;
				auto        worley_fbm1   = worley_noise1 * 0.625F + worley_noise2 * 0.25F + worley_noise3 * 0.125F;
				auto        worley_fbm2   = worley_noise2 * 0.75F + worley_noise3 * 0.25F;
				// cellCount=4 -> worleyNoise4 is just noise due to sampling frequency=texel freque. So only take into account 2 frequencies for FBM
#else
					// 2 octaves
					float worleyNoise0 = (1.0f - Tileable3dNoise::WorleyNoise(coord, 4));
					float worleyNoise1 = (1.0f - Tileable3dNoise::WorleyNoise(coord, 7));
					float worleyNoise2 = (1.0f - Tileable3dNoise::WorleyNoise(coord, 10));
					float worleyNoise3 = (1.0f - Tileable3dNoise::WorleyNoise(coord, 13));
					float worleyFBM0 = worleyNoise0 * 0.75f + worleyNoise1 * 0.25f;
					float worleyFBM1 = worleyNoise1 * 0.75f + worleyNoise2 * 0.25f;
					float worleyFBM2 = worleyNoise2 * 0.75f + worleyNoise3 * 0.25f;
#endif

				auto addr = r * cloud_erosion_texture_size * cloud_erosion_texture_size + t * cloud_erosion_texture_size + s;
				addr *= 4;
				cloud_erosion_texels[addr]     = unsigned char(255.0f * worley_fbm0);
				cloud_erosion_texels[addr + 1] = unsigned char(255.0f * worley_fbm1);
				cloud_erosion_texels[addr + 2] = unsigned char(255.0f * worley_fbm2);
				cloud_erosion_texels[addr + 3] = unsigned char(255.0f);

				float value = 0.0;
				{
					value = worley_fbm0 * 0.625F + worley_fbm1 * 0.25F + worley_fbm2 * 0.125F;
				}
				cloud_erosion_texels_packed[addr]     = unsigned char(255.0f * value);
				cloud_erosion_texels_packed[addr + 1] = unsigned char(255.0f * value);
				cloud_erosion_texels_packed[addr + 2] = unsigned char(255.0f * value);
				cloud_erosion_texels_packed[addr + 3] = unsigned char(255.0f);
			}
		}
	}

	{
		auto width  = cloud_erosion_texture_size * cloud_erosion_texture_size;
		auto height = cloud_erosion_texture_size;
		stbi_write_tga("noiseErosion.tga", width, height, 4, cloud_erosion_texels);
		stbi_write_tga("noiseErosionPacked.tga", width, height, 4, cloud_erosion_texels_packed);
	}

	free(cloud_base_shape_texels);
	free(cloud_base_shape_texels_packed);
	free(cloud_erosion_texels);
	free(cloud_erosion_texels_packed);
}
