#include "noise.hpp"
#include <algorithm>

float hash(float n)
{
	return glm::fract(sin(n + 1.951F) * 43758.5453F);
}

float noise(const glm::vec3& x)
{
	const auto p = floor(x);
	auto       f = fract(x);

	f            = f * f * (glm::vec3(3.0F) - glm::vec3(2.0F) * f);
	const auto n = p.x + p.y * 57.0F + 113.0F * p.z;
	return glm::mix(
	                glm::mix(
	                         glm::mix(hash(n + 0.0F), hash(n + 1.0F), f.x),
	                         glm::mix(hash(n + 57.0F), hash(n + 58.0F), f.x),
	                         f.y),
	                glm::mix(
	                         glm::mix(hash(n + 113.0F), hash(n + 114.0F), f.x),
	                         glm::mix(hash(n + 170.0F), hash(n + 171.0F), f.x),
	                         f.y),
	                f.z);
}

float cells(const glm::vec3& p, float cell_count)
{
	const auto p_cell = p * cell_count;
	float      d      = 1.0e10;
	for (auto xo = -1; xo <= 1; xo++)
	{
		for (auto yo = -1; yo <= 1; yo++)
		{
			for (auto zo = -1; zo <= 1; zo++)
			{
				auto tp = floor(p_cell) + glm::vec3(xo, yo, zo);

				tp = p_cell - tp - noise(mod(tp, cell_count / 1));

				d = glm::min(d, dot(tp, tp));
			}
		}
	}

	return std::clamp(d, 0.0F, 1.0F);
}

float worley(const glm::vec3& point, float cell_count)
{
	return cells(point, cell_count);
}

float perlin(const glm::vec3& p, float frequency, int octave_count)
{
	constexpr float octave_frequency_factor = 2;

	float sum{};
	float weight_sum{};
	auto  weight = 0.5F;

	for (auto octave = 0; octave < octave_count; octave++)
	{
		// todo: is 3d perlin bugged in glm?
		auto       point     = p * frequency;
		const auto noise_val = glm::perlin(point, glm::vec3(frequency));

		sum += noise_val;
		weight_sum += weight;

		weight *= weight;
		frequency *= octave_frequency_factor;
	}

	const auto noise = sum / weight_sum * 0.5F + 0.5F;
	return std::clamp(noise, 0.0F, 1.0F);
}
