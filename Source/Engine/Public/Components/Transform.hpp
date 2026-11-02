#pragma once

#include "Math/Rotator.hpp"

#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/quaternion_double.hpp"
#include "glm/ext/vector_double3.hpp"

namespace Hellfire::Engine::Components
{
class Transform final
{
public:
    static constexpr glm::dvec3 WORLD_FORWARD{1.0, 0.0, 0.0};
    static constexpr glm::dvec3 WORLD_RIGHT{0.0, 1.0, 0.0};
    static constexpr glm::dvec3 WORLD_UP{0.0, 0.0, 1.0};

    constexpr Transform() noexcept
        : m_position(0.0)
        , m_rotator()
        , m_scale(1.0)
    {
    }

    constexpr Transform(const glm::dvec3& position, const Math::Rotator& rotator, const glm::dvec3& scale) noexcept
        : m_position(position)
        , m_rotator(rotator)
        , m_scale(scale)
    {
    }

    constexpr const glm::dvec3& position() const noexcept
    {
        return m_position;
    }

    constexpr glm::dvec3& position() noexcept
    {
        return m_position;
    }

    constexpr const Math::Rotator& rotator() const noexcept
    {
        return m_rotator;
    }

    constexpr Math::Rotator& rotator() noexcept
    {
        return m_rotator;
    }

    constexpr const glm::dvec3& scale() const noexcept
    {
        return m_scale;
    }

    constexpr glm::dvec3& scale() noexcept
    {
        return m_scale;
    }

    [[nodiscard]] glm::dvec3 forward() const noexcept
    {
        return m_rotator.quaternion() * WORLD_FORWARD;
    }

    [[nodiscard]] glm::dvec3 right() const noexcept
    {
        return m_rotator.quaternion() * WORLD_RIGHT;
    }

    [[nodiscard]] glm::dvec3 up() const noexcept
    {
        return m_rotator.quaternion() * WORLD_UP;
    }

    [[nodiscard]] static Transform fromMatrix(const glm::dmat4& matrix) noexcept;

    [[nodiscard]] glm::dmat4 model() const noexcept;

    constexpr bool operator==(const Transform& other) const noexcept = default;

private:
    glm::dvec3 m_position;
    Math::Rotator m_rotator;
    glm::dvec3 m_scale;
};
} // namespace Hellfire::Engine::Components
