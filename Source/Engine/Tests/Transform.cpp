#include "Components/Transform.hpp"
#include "Logger.hpp"
#include "Math/Rotator.hpp"
#include "StaticOnly.hpp"

#include "catch2/catch_test_macros.hpp"
#include "catch2/generators/catch_generators.hpp"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/ext/vector_double4.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/trigonometric.hpp"

#include <cmath>
#include <cstddef>
#include <memory>

namespace Hellfire::Engine::Components::TransformTests
{
constexpr double ROUND_TRIP_TOLERANCE = 1e-9;
constexpr double SNAPPED_ORIENTATION_LIMIT = 0.12;
constexpr glm::dvec3 POSITION{10.0, -20.0, 30.0};
constexpr glm::dvec3 SCALE{0.5, 2.0, 3.0};

class Matrix final : public StaticOnly
{
public:
    // Frobenius norm of the difference, so a NaN anywhere fails the comparison.
    static double distance(const glm::dmat4& first, const glm::dmat4& second)
    {
        const glm::dmat4 difference = first - second;
        double sumOfSquares = 0.0;
        for (glm::length_t column = 0; column < difference.length(); ++column)
        {
            sumOfSquares += glm::dot(difference[column], difference[column]);
        }

        return std::sqrt(sumOfSquares);
    }
};

class CountingOutput final : public LogOutput
{
public:
    explicit CountingOutput(std::size_t& messageCount) noexcept
        : m_messageCount(messageCount)
    {
    }

    void write(const LogMessage&) noexcept override
    {
        ++m_messageCount;
    }

private:
    std::size_t& m_messageCount;
};

class DefaultLoggerFixture
{
public:
    DefaultLoggerFixture()
        : m_messageCount(0)
        , m_logger()
    {
        m_logger.addOutput(std::make_unique<CountingOutput>(m_messageCount));
        m_logger.makeDefault();
    }

protected:
    std::size_t m_messageCount;
    Logger m_logger;
};

TEST_CASE("A default Transform sits at the origin with no rotation and unit scale", "[Transform]")
{
    const Transform transform{};

    CHECK(transform.position() == glm::dvec3{0.0});
    CHECK(transform.rotator() == Math::Rotator{});
    CHECK(transform.scale() == glm::dvec3{1.0});
}

TEST_CASE("A Transform built from values works in constant evaluation", "[Transform]")
{
    constexpr Transform transform{POSITION, Math::Rotator{10.0, 20.0, 30.0}, SCALE};

    STATIC_CHECK(transform.position() == POSITION);
    STATIC_CHECK(transform.rotator() == Math::Rotator{10.0, 20.0, 30.0});
    STATIC_CHECK(transform.scale() == SCALE);
    STATIC_CHECK(transform == Transform{POSITION, Math::Rotator{10.0, 20.0, 30.0}, SCALE});
    STATIC_CHECK(transform != Transform{});
}

TEST_CASE("operator== compares every component", "[Transform]")
{
    constexpr Transform transform{POSITION, Math::Rotator{10.0, 20.0, 30.0}, SCALE};

    STATIC_CHECK(transform != Transform{glm::dvec3{0.0}, Math::Rotator{10.0, 20.0, 30.0}, SCALE});
    STATIC_CHECK(transform != Transform{POSITION, Math::Rotator{10.0, 20.0, 31.0}, SCALE});
    STATIC_CHECK(transform != Transform{POSITION, Math::Rotator{10.0, 20.0, 30.0}, glm::dvec3{1.0}});
}

TEST_CASE("Transform components can be written through their accessors", "[Transform]")
{
    Transform transform{};

    transform.position() = POSITION;
    transform.rotator().yaw() = 45.0;
    transform.scale() = SCALE;

    CHECK(transform == Transform{POSITION, Math::Rotator{0.0, 45.0, 0.0}, SCALE});
}

TEST_CASE("Quarter turns move forward right and up onto world axes", "[Transform]")
{
    const auto [rotator, expectedForward, expectedRight, expectedUp] = GENERATE(table<Math::Rotator, glm::dvec3, glm::dvec3, glm::dvec3>({
        {Math::Rotator{}, Transform::WORLD_FORWARD, Transform::WORLD_RIGHT, Transform::WORLD_UP},
        {Math::Rotator{90.0, 0.0, 0.0}, Transform::WORLD_UP, Transform::WORLD_RIGHT, -Transform::WORLD_FORWARD},
        {Math::Rotator{0.0, 90.0, 0.0}, Transform::WORLD_RIGHT, -Transform::WORLD_FORWARD, Transform::WORLD_UP},
        {Math::Rotator{0.0, 0.0, 90.0}, Transform::WORLD_FORWARD, -Transform::WORLD_UP, Transform::WORLD_RIGHT},
    }));
    const Transform transform{POSITION, rotator, SCALE};

    CHECK(glm::distance(transform.forward(), expectedForward) < ROUND_TRIP_TOLERANCE);
    CHECK(glm::distance(transform.right(), expectedRight) < ROUND_TRIP_TOLERANCE);
    CHECK(glm::distance(transform.up(), expectedUp) < ROUND_TRIP_TOLERANCE);
}

TEST_CASE("forward right and up stay an orthonormal basis", "[Transform]")
{
    const double pitch = GENERATE(-90.0, -45.0, 0.0, 30.0, 90.0);
    const double yaw = GENERATE(-135.0, 0.0, 60.0, 180.0);
    const double roll = GENERATE(-120.0, 0.0, 45.0);
    const Transform transform{POSITION, Math::Rotator{pitch, yaw, roll}, SCALE};

    const glm::dvec3 forward = transform.forward();
    const glm::dvec3 right = transform.right();
    const glm::dvec3 up = transform.up();

    CHECK(std::abs(glm::length(forward) - 1.0) < ROUND_TRIP_TOLERANCE);
    CHECK(std::abs(glm::length(right) - 1.0) < ROUND_TRIP_TOLERANCE);
    CHECK(std::abs(glm::length(up) - 1.0) < ROUND_TRIP_TOLERANCE);
    CHECK(glm::distance(glm::cross(forward, right), up) < ROUND_TRIP_TOLERANCE);
}

TEST_CASE("Negative and zero scale do not change forward right and up", "[Transform]")
{
    const Math::Rotator rotator{30.0, 40.0, 50.0};
    const Transform unit{POSITION, rotator, glm::dvec3{1.0}};
    const Transform scaled{POSITION, rotator, glm::dvec3{-2.0, 0.0, 5.0}};

    CHECK(scaled.forward() == unit.forward());
    CHECK(scaled.right() == unit.right());
    CHECK(scaled.up() == unit.up());
}

TEST_CASE("A default Transform has the identity model matrix", "[Transform]")
{
    CHECK(Transform{}.model() == glm::dmat4{1.0});
}

TEST_CASE("model scales then rotates then translates a point", "[Transform]")
{
    const Transform transform{glm::dvec3{10.0, 20.0, 30.0}, Math::Rotator{0.0, 90.0, 0.0}, glm::dvec3{2.0, 3.0, 4.0}};

    const glm::dvec4 result = transform.model() * glm::dvec4{1.0, 1.0, 1.0, 1.0};

    CHECK(glm::distance(glm::dvec3{result}, glm::dvec3{7.0, 22.0, 34.0}) < ROUND_TRIP_TOLERANCE);
    CHECK(result.w == 1.0);
}

TEST_CASE("model columns are the scaled axes and the position", "[Transform]")
{
    const Math::Rotator rotator = GENERATE(Math::Rotator{}, Math::Rotator{30.0, 40.0, 50.0}, Math::Rotator{-60.0, 170.0, -120.0}, Math::Rotator{90.0, 45.0, 0.0});
    const Transform transform{POSITION, rotator, SCALE};

    const glm::dmat4 model = transform.model();

    CHECK(glm::distance(glm::dvec3{model[0]}, transform.forward() * SCALE.x) < ROUND_TRIP_TOLERANCE);
    CHECK(glm::distance(glm::dvec3{model[1]}, transform.right() * SCALE.y) < ROUND_TRIP_TOLERANCE);
    CHECK(glm::distance(glm::dvec3{model[2]}, transform.up() * SCALE.z) < ROUND_TRIP_TOLERANCE);
    CHECK(model[3] == glm::dvec4{POSITION, 1.0});
    CHECK(model[0].w == 0.0);
    CHECK(model[1].w == 0.0);
    CHECK(model[2].w == 0.0);
}

TEST_CASE("model equals translation times rotation times scale", "[Transform]")
{
    const Math::Rotator rotator = GENERATE(Math::Rotator{}, Math::Rotator{30.0, 40.0, 50.0}, Math::Rotator{-60.0, 170.0, -120.0}, Math::Rotator{90.0, 45.0, 0.0});
    const glm::dvec3 scale = GENERATE(glm::dvec3{1.0}, SCALE, glm::dvec3{-1.0, 2.0, -3.0}, glm::dvec3{0.0, 1.0, 1.0});
    const Transform transform{POSITION, rotator, scale};

    const glm::dmat4 expected = glm::translate(glm::dmat4{1.0}, POSITION) * glm::mat4_cast(rotator.quaternion()) * glm::scale(glm::dmat4{1.0}, scale);

    CHECK(Matrix::distance(transform.model(), expected) < ROUND_TRIP_TOLERANCE);
}

TEST_CASE("fromMatrix recovers a Transform with positive scale", "[Transform]")
{
    const double pitch = GENERATE(-60.0, 0.0, 45.0);
    const double yaw = GENERATE(-170.0, 0.0, 90.0);
    const double roll = GENERATE(-120.0, 0.0, 30.0);
    const glm::dvec3 scale = GENERATE(glm::dvec3{1.0}, SCALE, glm::dvec3{100.0, 0.01, 1.0});
    const Transform transform{POSITION, Math::Rotator{pitch, yaw, roll}, scale};

    const Transform result = Transform::fromMatrix(transform.model());

    CHECK(result.position() == POSITION);
    CHECK(result.rotator().equals(transform.rotator(), ROUND_TRIP_TOLERANCE));
    CHECK(glm::distance(result.scale(), scale) < ROUND_TRIP_TOLERANCE);
}

TEST_CASE("fromMatrix moves a reflection onto the X scale", "[Transform]")
{
    const auto [scale, expectedScale, expectedRotator] = GENERATE(table<glm::dvec3, glm::dvec3, Math::Rotator>({
        {glm::dvec3{-1.0, 1.0, 1.0}, glm::dvec3{-1.0, 1.0, 1.0}, Math::Rotator{}},
        {glm::dvec3{1.0, -1.0, 1.0}, glm::dvec3{-1.0, 1.0, 1.0}, Math::Rotator{0.0, 180.0, 0.0}},
        {glm::dvec3{1.0, 1.0, -1.0}, glm::dvec3{-1.0, 1.0, 1.0}, Math::Rotator{0.0, 180.0, 180.0}},
        {glm::dvec3{-2.0, -3.0, -4.0}, glm::dvec3{-2.0, 3.0, 4.0}, Math::Rotator{0.0, 0.0, 180.0}},
    }));
    const Transform transform{POSITION, Math::Rotator{}, scale};

    const Transform result = Transform::fromMatrix(transform.model());

    CHECK(glm::distance(result.scale(), expectedScale) < ROUND_TRIP_TOLERANCE);
    CHECK(result.rotator().equals(expectedRotator, ROUND_TRIP_TOLERANCE));
    CHECK(Matrix::distance(result.model(), transform.model()) < ROUND_TRIP_TOLERANCE);
}

TEST_CASE("fromMatrix keeps the model matrix of a Transform with negative scale", "[Transform]")
{
    const Math::Rotator rotator = GENERATE(Math::Rotator{30.0, 40.0, 50.0}, Math::Rotator{-60.0, 170.0, -120.0}, Math::Rotator{90.0, 45.0, 0.0});
    const auto [scale, expectedScale] = GENERATE(table<glm::dvec3, glm::dvec3>({
        {glm::dvec3{1.0, -2.0, 3.0}, glm::dvec3{-1.0, 2.0, 3.0}},
        {glm::dvec3{-1.0, -2.0, -3.0}, glm::dvec3{-1.0, 2.0, 3.0}},
        {glm::dvec3{-1.0, -2.0, 3.0}, glm::dvec3{1.0, 2.0, 3.0}},
        {glm::dvec3{1.0, -2.0, -3.0}, glm::dvec3{1.0, 2.0, 3.0}},
    }));
    const Transform transform{POSITION, rotator, scale};

    const Transform result = Transform::fromMatrix(transform.model());

    CHECK(glm::distance(result.scale(), expectedScale) < ROUND_TRIP_TOLERANCE);
    CHECK(Matrix::distance(result.model(), transform.model()) < ROUND_TRIP_TOLERANCE);
}

TEST_CASE("fromMatrix snaps to a pole like Rotator does", "[Transform]")
{
    const double pitch = GENERATE(-89.95, 89.95);
    const Transform transform{POSITION, Math::Rotator{pitch, 45.0, 30.0}, SCALE};
    const double minimumAxisAlignment = std::cos(glm::radians(SNAPPED_ORIENTATION_LIMIT));

    const Transform result = Transform::fromMatrix(transform.model());

    CHECK(std::abs(result.rotator().pitch()) == 90.0);
    CHECK(glm::dot(result.forward(), transform.forward()) > minimumAxisAlignment);
    CHECK(glm::dot(result.right(), transform.right()) > minimumAxisAlignment);
    CHECK(glm::dot(result.up(), transform.up()) > minimumAxisAlignment);
    CHECK(result.position() == POSITION);
    CHECK(glm::distance(result.scale(), SCALE) < ROUND_TRIP_TOLERANCE);
}

TEST_CASE_METHOD(DefaultLoggerFixture, "fromMatrix reports a degenerate axis and keeps position and scale", "[Transform]")
{
    const glm::dvec3 scale = GENERATE(glm::dvec3{0.0, 1.0, 1.0}, glm::dvec3{1.0, 0.0, 1.0}, glm::dvec3{1.0, 1.0, 0.0}, glm::dvec3{0.0}, glm::dvec3{1e-9, 2.0, 3.0});
    const Transform transform{POSITION, Math::Rotator{30.0, 40.0, 50.0}, scale};

    const Transform result = Transform::fromMatrix(transform.model());

    CHECK(m_messageCount == 1u);
    CHECK(result.position() == POSITION);
    CHECK(result.rotator() == Math::Rotator{});
    CHECK(glm::distance(result.scale(), scale) < ROUND_TRIP_TOLERANCE);
}

TEST_CASE_METHOD(DefaultLoggerFixture, "fromMatrix accepts a tiny scale above the degenerate limit", "[Transform]")
{
    const Transform transform{POSITION, Math::Rotator{30.0, 40.0, 50.0}, glm::dvec3{1e-6, 2.0, 3.0}};

    const Transform result = Transform::fromMatrix(transform.model());

    CHECK(m_messageCount == 0u);
    CHECK(result.rotator().equals(transform.rotator(), ROUND_TRIP_TOLERANCE));
    CHECK(glm::distance(result.scale(), transform.scale()) < ROUND_TRIP_TOLERANCE);
}

TEST_CASE("operator== compares components, not the transformation", "[Transform]")
{
    const Transform transform{POSITION, Math::Rotator{}, glm::dvec3{1.0, -1.0, 1.0}};

    const Transform result = Transform::fromMatrix(transform.model());

    CHECK(result != transform);
    CHECK(Matrix::distance(result.model(), transform.model()) < ROUND_TRIP_TOLERANCE);
}
} // namespace Hellfire::Engine::Components::TransformTests
