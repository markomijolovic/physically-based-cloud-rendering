#include "transforms.hpp"

glm::mat4x4 translate(float x, float y, float z)
{
	glm::mat4x4 retval{1.0F};
	retval[3][0] = x;
	retval[3][1] = y;
	retval[3][2] = z;
	return retval;
}

glm::mat4x4 translate(const glm::vec3& vec3)
{
	return translate(vec3.x, vec3.y, vec3.z);
}

glm::mat4x4 scale(float x, float y, float z)
{
	glm::mat4x4 retval{1.0F};
	retval[0][0] *= x;
	retval[1][1] *= y;
	retval[2][2] *= z;
	return retval;
}

glm::mat4x4 scale(const glm::vec3& vec3)
{
	return scale(vec3.x, vec3.y, vec3.z);
}

glm::mat4x4 scale(float x)
{
	return scale(x, x, x);
}

glm::mat4x4 rotate(float angle, const glm::vec3& axis_)
{
	const auto axis = normalize(axis_);
	const auto s    = std::sin(angle);
	const auto c    = std::cos(angle);
	const auto oc   = 1.0F - c;

	return
	{
		oc * axis.x * axis.x + c, oc * axis.x * axis.y + axis.z * s, oc * axis.z * axis.x - axis.y * s, 0.0F,
		oc * axis.x * axis.y - axis.z * s, oc * axis.y * axis.y + c, oc * axis.y * axis.z + axis.x * s, 0.0F,
		oc * axis.z * axis.x + axis.y * s, oc * axis.y * axis.z - axis.x * s, oc * axis.z * axis.z + c, 0.0F,
		0.0F, 0.0F, 0.0F, 1.0F
	};
}

glm::mat4x4 perspective(float fov_y, float aspect, float z_near, float z_far)
{
	const auto theta = radians(fov_y / 2);
	const auto d     = cos(theta) / sin(theta);
	const auto a     = -(z_far + z_near) / (z_far - z_near);
	const auto b     = -(2 * z_far * z_near) / (z_far - z_near);
	return
	{
		d / aspect, 0.0F, 0.0F, 0.0F,
		0.0F, d, 0.0F, 0.0F,
		0.0F, 0.0F, a, -1.0F,
		0.0F, 0.0F, b, 0.0F
	};
}
