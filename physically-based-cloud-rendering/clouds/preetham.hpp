#pragma once

#include <glm/mat3x3.hpp>

#include <glm/vec3.hpp>

#include "transforms.hpp"

inline glm::vec3 YxyToXYZ(glm::vec3 Yxy)
{
	float Y = Yxy.r;
	float x = Yxy.g;
	float y = Yxy.b;

	float X = x * (Y / y);
	float Z = (1.0 - x - y) * (Y / y);

	return glm::vec3(X, Y, Z);
}

inline glm::vec3 XYZToRGB(glm::vec3 XYZ)
{
	// CIE/E
	glm::mat3 M = glm::mat3
	(
		2.3706743, -0.9000405, -0.4706338,
		-0.5138850, 1.4253036, 0.0885814,
		0.0052982, -0.0146949, 1.0093968
	);

	return XYZ * M;
}


inline float saturatedDot(glm::vec3 a, glm::vec3 b)
{
	return glm::max(dot(a, b), 0.0F);
}

inline glm::vec3 YxyToRGB(glm::vec3 Yxy)
{
	glm::vec3 XYZ = YxyToXYZ(Yxy);
	glm::vec3 RGB = XYZToRGB(XYZ);
	return RGB;
}

inline void calculatePerezDistribution(float t, glm::vec3& A, glm::vec3& B, glm::vec3& C, glm::vec3& D, glm::vec3& E)
{
	A = glm::vec3(0.1787 * t - 1.4630, -0.0193 * t - 0.2592, -0.0167 * t - 0.2608);
	B = glm::vec3(-0.3554 * t + 0.4275, -0.0665 * t + 0.0008, -0.0950 * t + 0.0092);
	C = glm::vec3(-0.0227 * t + 5.3251, -0.0004 * t + 0.2125, -0.0079 * t + 0.2102);
	D = glm::vec3(0.1206 * t - 2.5771, -0.0641 * t - 0.8989, -0.0441 * t - 1.6537);
	E = glm::vec3(-0.0670 * t + 0.3703, -0.0033 * t + 0.0452, -0.0109 * t + 0.0529);
}

inline glm::vec3 calculateZenithLuminanceYxy(float t, float thetaS)
{
	float chi = (4.0 / 9.0 - t / 120.0) * (pi - 2.0 * thetaS);
	float Yz = (4.0453 * t - 4.9710) * tan(chi) - 0.2155 * t + 2.4192;

	float theta2 = thetaS * thetaS;
	float theta3 = theta2 * thetaS;
	float T = t;
	float T2 = t * t;

	float xz =
		(0.00165 * theta3 - 0.00375 * theta2 + 0.00209 * thetaS + 0.0) * T2 +
		(-0.02903 * theta3 + 0.06377 * theta2 - 0.03202 * thetaS + 0.00394) * T +
		(0.11693 * theta3 - 0.21196 * theta2 + 0.06052 * thetaS + 0.25886);

	float yz =
		(0.00275 * theta3 - 0.00610 * theta2 + 0.00317 * thetaS + 0.0) * T2 +
		(-0.04214 * theta3 + 0.08970 * theta2 - 0.04153 * thetaS + 0.00516) * T +
		(0.15346 * theta3 - 0.26756 * theta2 + 0.06670 * thetaS + 0.26688);

	return glm::vec3(Yz, xz, yz);
}

inline glm::vec3 calculatePerezLuminanceYxy(float theta, float gamma, glm::vec3 A, glm::vec3 B, glm::vec3 C, glm::vec3 D, glm::vec3 E)
{
	return (1.0F + A * exp(B / cos(theta))) * (1.0F + C * exp(D * gamma) + E * cos(gamma) * cos(gamma));
}

inline glm::vec3 calculateSkyLuminanceRGB(glm::vec3 s, glm::vec3 e, float t)
{
	glm::vec3 A, B, C, D, E;
	calculatePerezDistribution(t, A, B, C, D, E);

	float thetaS = acos(saturatedDot(s, glm::vec3(0, 1, 0)));
	float thetaE = acos(saturatedDot(e, glm::vec3(0, 1, 0)));
	float gammaE = acos(saturatedDot(s, e));

	glm::vec3 Yz = calculateZenithLuminanceYxy(t, thetaS);

	glm::vec3 fThetaGamma = calculatePerezLuminanceYxy(thetaE, gammaE, A, B, C, D, E);
	glm::vec3 fZeroThetaS = calculatePerezLuminanceYxy(0.0, thetaS, A, B, C, D, E);

	glm::vec3 Yp = Yz * (fThetaGamma / fZeroThetaS);

	return YxyToRGB(Yp);
}
