#include "Math/Rotator.hpp"
#include "Assertion.hpp"

#include "glm/ext/quaternion_double.hpp"
#include "glm/ext/quaternion_geometric.hpp"
#include "glm/trigonometric.hpp"

#include <cmath>

namespace Hellfire::Engine::Math
{
namespace
{
constexpr double SINGULARITY_THRESHOLD = 0.4999995;
constexpr double UNIT_LENGTH_TOLERANCE = 1e-6;
} // namespace

Rotator::Rotator(const glm::dquat& rotation) noexcept
{
    HELLFIRE_VERIFY_LOG(std::abs(glm::dot(rotation, rotation) - 1.0) < UNIT_LENGTH_TOLERANCE, "Rotator built from a non-unit quaternion");

    const double singularityTest = rotation.z * rotation.x - rotation.w * rotation.y;
    const double yawY = 2.0 * (rotation.w * rotation.z + rotation.x * rotation.y);
    const double yawX = 1.0 - 2.0 * (rotation.y * rotation.y + rotation.z * rotation.z);

    if (singularityTest < -SINGULARITY_THRESHOLD)
    {
        m_pitch = -90.0;
        m_yaw = normalizeAxis(-2.0 * glm::degrees(std::atan2(rotation.x, rotation.w)));
        m_roll = 0.0;
    }
    else if (singularityTest > SINGULARITY_THRESHOLD)
    {
        m_pitch = 90.0;
        m_yaw = normalizeAxis(2.0 * glm::degrees(std::atan2(rotation.x, rotation.w)));
        m_roll = 0.0;
    }
    else
    {
        m_pitch = glm::degrees(std::asin(2.0 * singularityTest));
        m_yaw = glm::degrees(std::atan2(yawY, yawX));
        m_roll = glm::degrees(std::atan2(-2.0 * (rotation.w * rotation.x + rotation.y * rotation.z), 1.0 - 2.0 * (rotation.x * rotation.x + rotation.y * rotation.y)));
    }
}

glm::dquat Rotator::quaternion() const noexcept
{
    const double halfPitch = glm::radians(std::fmod(m_pitch, 360.0)) * 0.5;
    const double halfYaw = glm::radians(std::fmod(m_yaw, 360.0)) * 0.5;
    const double halfRoll = glm::radians(std::fmod(m_roll, 360.0)) * 0.5;

    const double sinPitch = std::sin(halfPitch);
    const double cosPitch = std::cos(halfPitch);
    const double sinYaw = std::sin(halfYaw);
    const double cosYaw = std::cos(halfYaw);
    const double sinRoll = std::sin(halfRoll);
    const double cosRoll = std::cos(halfRoll);

    return glm::dquat::wxyz(cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw,
                            cosRoll * sinPitch * sinYaw - sinRoll * cosPitch * cosYaw,
                            -cosRoll * sinPitch * cosYaw - sinRoll * cosPitch * sinYaw,
                            cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw);
}

bool Rotator::equals(const Rotator& other, double tolerance) const noexcept
{
    return std::abs(normalizeAxis(m_pitch - other.m_pitch)) <= tolerance && std::abs(normalizeAxis(m_yaw - other.m_yaw)) <= tolerance && std::abs(normalizeAxis(m_roll - other.m_roll)) <= tolerance;
}

double Rotator::normalizeAxis(double angle) noexcept
{
    angle = std::fmod(angle, 360.0);
    if (angle < 0.0)
    {
        angle += 360.0;
    }

    if (angle > 180.0)
    {
        angle -= 360.0;
    }

    return angle;
}
} // namespace Hellfire::Engine::Math
