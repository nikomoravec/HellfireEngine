#include "Components/Transform.hpp"

#include "Assertion.hpp"

#include "glm/ext/matrix_double3x3.hpp"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/component_wise.hpp"
#include "glm/matrix.hpp"

namespace
{
constexpr double MINIMUM_AXIS_LENGTH = 1e-8;
} // namespace

namespace Hellfire::Engine::Components
{
Transform Transform::fromMatrix(const glm::dmat4& matrix) noexcept
{
    const glm::dvec3 position{matrix[3]};

    const glm::dvec3 axisX{matrix[0]};
    const glm::dvec3 axisY{matrix[1]};
    const glm::dvec3 axisZ{matrix[2]};

    glm::dvec3 scale{glm::length(axisX), glm::length(axisY), glm::length(axisZ)};
    HELLFIRE_VERIFY_RETURN_VALUE(glm::compMin(scale) > MINIMUM_AXIS_LENGTH, (Transform{position, Math::Rotator{}, scale}), "Matrix has a degenerate axis, scale ({}, {}, {})", scale.x, scale.y, scale.z);

    if (glm::dot(glm::cross(axisX, axisY), axisZ) < 0.0)
    {
        scale.x = -scale.x;
    }

    const glm::dmat3 rotation{axisX / scale.x, axisY / scale.y, axisZ / scale.z};
    return Transform{position, Math::Rotator{glm::normalize(glm::quat_cast(rotation))}, scale};
}

glm::dmat4 Transform::model() const noexcept
{
    glm::dmat4 matrix = glm::mat4_cast(m_rotator.quaternion());
    matrix[0] *= m_scale.x;
    matrix[1] *= m_scale.y;
    matrix[2] *= m_scale.z;
    matrix[3] = glm::dvec4{m_position, 1.0};

    return matrix;
}
} // namespace Hellfire::Engine::Components
