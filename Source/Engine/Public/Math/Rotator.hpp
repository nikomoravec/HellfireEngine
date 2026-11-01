#pragma once

#include "glm/fwd.hpp"

namespace Hellfire::Engine::Math
{
// +X forward, +Y right, +Z up, left-handed, angles in degrees
class Rotator
{
public:
    constexpr Rotator() noexcept
        : m_pitch(0.0)
        , m_yaw(0.0)
        , m_roll(0.0)
    {
    }

    constexpr Rotator(double pitch, double yaw, double roll) noexcept
        : m_pitch(pitch)
        , m_yaw(yaw)
        , m_roll(roll)
    {
    }

    // Port of UE FQuat4d::Rotator()
    explicit Rotator(const glm::dquat& rotation) noexcept;

    [[nodiscard]] glm::dquat quaternion() const noexcept;

    // Compares each axis modulo 360. Two rotators for the same orientation can still differ, for example at a pole.
    [[nodiscard]] bool equals(const Rotator& other, double tolerance) const noexcept;

    constexpr double& pitch() noexcept
    {
        return m_pitch;
    }

    constexpr double& yaw() noexcept
    {
        return m_yaw;
    }

    constexpr double& roll() noexcept
    {
        return m_roll;
    }

    constexpr double pitch() const noexcept
    {
        return m_pitch;
    }

    constexpr double yaw() const noexcept
    {
        return m_yaw;
    }

    constexpr double roll() const noexcept
    {
        return m_roll;
    }

    constexpr bool operator==(const Rotator& other) const noexcept = default;

    [[nodiscard]] static double normalizeAxis(double angle) noexcept;

private:
    double m_pitch;
    double m_yaw;
    double m_roll;
};
} // namespace Hellfire::Engine::Math
