#pragma once

#include "transforms.hpp"

#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

[[nodiscard]] inline auto Yxy_to_XYZ(glm::vec3 Yxy) -> glm::vec3
{
    const auto Y = Yxy.r;
    const auto x = Yxy.g;
    const auto y = Yxy.b;

    const auto X = x * (Y / y);
    const auto Z = (1.0F - x - y) * (Y / y);

    return glm::vec3(X, Y, Z);
}

[[nodiscard]] inline auto XYZ_to_RGB(glm::vec3 XYZ) -> glm::vec3
{
    // CIE/E
    const auto M = glm::mat3(2.3706743F,
                             -0.9000405F,
                             -0.4706338F,
                             -0.5138850F,
                             1.4253036F,
                             0.0885814F,
                             0.0052982F,
                             -0.0146949F,
                             1.0093968F);

    return XYZ * M;
}

[[nodiscard]] inline auto saturated_dot(glm::vec3 a, glm::vec3 b) -> float { return glm::max(dot(a, b), 0.0F); }

[[nodiscard]] inline auto Yxy_to_RGB(glm::vec3 Yxy) -> glm::vec3
{
    const auto XYZ = Yxy_to_XYZ(Yxy);
    const auto RGB = XYZ_to_RGB(XYZ);
    return RGB;
}

inline auto calculate_perez_distribution(float      t,
                                         glm::vec3 &A,
                                         glm::vec3 &B,
                                         glm::vec3 &C,
                                         glm::vec3 &D,
                                         glm::vec3 &E) -> void
{
    A = glm::vec3(0.1787F * t - 1.4630F, -0.0193F * t - 0.2592F, -0.0167F * t - 0.2608F);
    B = glm::vec3(-0.3554F * t + 0.4275F, -0.0665F * t + 0.0008F, -0.0950F * t + 0.0092F);
    C = glm::vec3(-0.0227F * t + 5.3251F, -0.0004F * t + 0.2125F, -0.0079F * t + 0.2102F);
    D = glm::vec3(0.1206F * t - 2.5771F, -0.0641F * t - 0.8989F, -0.0441F * t - 1.6537F);
    E = glm::vec3(-0.0670F * t + 0.3703F, -0.0033F * t + 0.0452F, -0.0109F * t + 0.0529F);
}

[[nodiscard]] inline auto calculate_zenith_luminance_Yxy(float t, float thetaS) -> glm::vec3
{
    const auto chi = (4.0F / 9.0F - t / 120.0F) * (pi - 2.0F * thetaS);
    const auto Yz  = (4.0453F * t - 4.9710F) * tan(chi) - 0.2155F * t + 2.4192F;

    const auto theta2 = thetaS * thetaS;
    const auto theta3 = theta2 * thetaS;
    const auto T      = t;
    const auto T2     = t * t;

    const auto xz = (0.00165F * theta3 - 0.00375F * theta2 + 0.00209F * thetaS + 0.0F) * T2 + (-0.02903F * theta3 + 0.06377F * theta2 - 0.03202F * thetaS + 0.00394F) * T + (0.11693F * theta3 - 0.21196F * theta2 + 0.06052F * thetaS + 0.25886F);

    const auto yz = (0.00275F * theta3 - 0.00610F * theta2 + 0.00317F * thetaS + 0.0F) * T2 + (-0.04214F * theta3 + 0.08970F * theta2 - 0.04153F * thetaS + 0.00516F) * T + (0.15346F * theta3 - 0.26756F * theta2 + 0.06670F * thetaS + 0.26688F);

    return glm::vec3(Yz, xz, yz);
}

[[nodiscard]] inline auto calculate_perez_luminance_Yxy(float     theta,
                                                        float     gamma,
                                                        glm::vec3 A,
                                                        glm::vec3 B,
                                                        glm::vec3 C,
                                                        glm::vec3 D,
                                                        glm::vec3 E) -> glm::vec3
{
    return (1.0F + A * exp(B / glm::max(0.00001F, cos(theta)))) * (1.0F + C * exp(D * gamma) + E * cos(gamma) * cos(gamma));
}

[[nodiscard]] inline auto calculate_sky_luminance_RGB(glm::vec3 s, glm::vec3 e, float t) -> glm::vec3
{
    glm::vec3 A;
    glm::vec3 B;
    glm::vec3 C;
    glm::vec3 D;
    glm::vec3 E;
    calculate_perez_distribution(t, A, B, C, D, E);

    auto thetaS = acos(saturated_dot(s, glm::vec3(0, 1, 0)));
    auto thetaE = acos(saturated_dot(e, glm::vec3(0, 1, 0)));
    auto gammaE = acos(saturated_dot(s, e));

    auto Yz = calculate_zenith_luminance_Yxy(t, thetaS);

    auto fThetaGamma = calculate_perez_luminance_Yxy(thetaE, gammaE, A, B, C, D, E);
    auto fZeroThetaS = calculate_perez_luminance_Yxy(0.0F, thetaS, A, B, C, D, E);

    auto Yp = Yz * (fThetaGamma / fZeroThetaS);

    return Yxy_to_RGB(Yp);
}
