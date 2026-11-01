#include "Math/Rotator.hpp"
#include "Logger.hpp"
#include "StaticOnly.hpp"

#include "catch2/catch_test_macros.hpp"
#include "catch2/generators/catch_generators.hpp"
#include "glm/ext/quaternion_common.hpp"
#include "glm/ext/quaternion_double.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/geometric.hpp"
#include "glm/trigonometric.hpp"

#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>

namespace Hellfire::Engine::Math::RotatorTests
{
constexpr double ROUND_TRIP_TOLERANCE = 1e-9;
constexpr double NEAR_POLE_MARGIN = 1e-4;
constexpr double SNAPPED_ORIENTATION_LIMIT = 0.12;
constexpr glm::dvec3 FORWARD{1.0, 0.0, 0.0};
constexpr glm::dvec3 RIGHT{0.0, 1.0, 0.0};

class Orientation final : public StaticOnly
{
public:
    // Stays accurate for tiny angles, unlike acos of the dot product.
    static double differenceDegrees(const glm::dquat& first, const glm::dquat& second)
    {
        const glm::dquat delta = glm::conjugate(first) * second;
        const double vectorLength = glm::length(glm::dvec3{delta.x, delta.y, delta.z});
        return glm::degrees(2.0 * std::atan2(vectorLength, std::abs(delta.w)));
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

TEST_CASE("A default Rotator is zero on every axis", "[Rotator]")
{
    const Rotator rotator{};

    CHECK(rotator.pitch() == 0.0);
    CHECK(rotator.yaw() == 0.0);
    CHECK(rotator.roll() == 0.0);
}

TEST_CASE("A Rotator built from numbers works in constant evaluation", "[Rotator]")
{
    constexpr Rotator rotator{10.0, 20.0, 30.0};

    STATIC_CHECK(rotator.pitch() == 10.0);
    STATIC_CHECK(rotator.yaw() == 20.0);
    STATIC_CHECK(rotator.roll() == 30.0);
    STATIC_CHECK(rotator == Rotator{10.0, 20.0, 30.0});
    STATIC_CHECK(rotator != Rotator{});
}

TEST_CASE("Rotator axes can be written through their accessors", "[Rotator]")
{
    Rotator rotator{};

    rotator.pitch() = 10.0;
    rotator.yaw() = 20.0;
    rotator.roll() = 30.0;

    CHECK(rotator == Rotator{10.0, 20.0, 30.0});
}

TEST_CASE("normalizeAxis wraps an angle into the half-open range from -180 to 180", "[Rotator]")
{
    const auto [angle, expected] = GENERATE(table<double, double>({
        {0.0, 0.0},
        {90.0, 90.0},
        {180.0, 180.0},
        {-180.0, 180.0},
        {181.0, -179.0},
        {270.0, -90.0},
        {-270.0, 90.0},
        {540.0, 180.0},
        {720.0, 0.0},
        {-1e-20, 0.0},
    }));

    CHECK(Rotator::normalizeAxis(angle) == expected);
}

TEST_CASE("normalizeAxis turns infinity into NaN", "[Rotator]")
{
    CHECK(std::isnan(Rotator::normalizeAxis(std::numeric_limits<double>::infinity())));
}

TEST_CASE("A zero Rotator is the identity quaternion", "[Rotator]")
{
    const glm::dquat rotation = Rotator{}.quaternion();

    CHECK(rotation.w == 1.0);
    CHECK(rotation.x == 0.0);
    CHECK(rotation.y == 0.0);
    CHECK(rotation.z == 0.0);
}

TEST_CASE("Positive pitch raises the forward axis towards +Z", "[Rotator]")
{
    const double angle = glm::radians(30.0);

    const glm::dvec3 result = Rotator{30.0, 0.0, 0.0}.quaternion() * FORWARD;

    CHECK(glm::distance(result, glm::dvec3{std::cos(angle), 0.0, std::sin(angle)}) < ROUND_TRIP_TOLERANCE);
}

TEST_CASE("Positive yaw turns the forward axis towards +Y", "[Rotator]")
{
    const double angle = glm::radians(30.0);

    const glm::dvec3 result = Rotator{0.0, 30.0, 0.0}.quaternion() * FORWARD;

    CHECK(glm::distance(result, glm::dvec3{std::cos(angle), std::sin(angle), 0.0}) < ROUND_TRIP_TOLERANCE);
}

TEST_CASE("Positive roll lowers the right axis towards -Z", "[Rotator]")
{
    const double angle = glm::radians(30.0);

    const glm::dvec3 result = Rotator{0.0, 0.0, 30.0}.quaternion() * RIGHT;

    CHECK(glm::distance(result, glm::dvec3{0.0, std::cos(angle), -std::sin(angle)}) < ROUND_TRIP_TOLERANCE);
}

TEST_CASE("quaternion ignores extra full turns", "[Rotator]")
{
    const glm::dquat wound = Rotator{30.0 + 720.0, 40.0 - 360.0, 50.0 + 1080.0}.quaternion();

    CHECK(Orientation::differenceDegrees(wound, Rotator{30.0, 40.0, 50.0}.quaternion()) < ROUND_TRIP_TOLERANCE);
}

TEST_CASE("A regular Rotator survives a quaternion round trip", "[Rotator]")
{
    const double pitch = GENERATE(-89.9, -45.0, -1.0, 0.0, 1.0, 45.0, 89.9);
    const double yaw = GENERATE(-179.0, -90.0, 0.0, 90.0, 179.0);
    const double roll = GENERATE(-179.0, -90.0, 0.0, 90.0, 179.0);
    const Rotator rotator{pitch, yaw, roll};

    CHECK(Rotator{rotator.quaternion()}.equals(rotator, ROUND_TRIP_TOLERANCE));
}

TEST_CASE("Snapping to a pole folds roll into yaw", "[Rotator]")
{
    const auto [input, expected] = GENERATE(table<Rotator, Rotator>({
        {Rotator{89.91, 45.0, 30.0}, Rotator{89.91, 45.0, 30.0}},
        {Rotator{89.92, 45.0, 30.0}, Rotator{90.0, 14.94346, 0.0}},
        {Rotator{90.0, 45.0, 30.0}, Rotator{90.0, 15.0, 0.0}},
        {Rotator{90.0, 45.0, 0.0}, Rotator{90.0, 45.0, 0.0}},
        {Rotator{90.0, 0.0, 0.0}, Rotator{90.0, 0.0, 0.0}},
        {Rotator{-89.91, 45.0, 30.0}, Rotator{-89.91, 45.0, 30.0}},
        {Rotator{-89.92, 45.0, 30.0}, Rotator{-90.0, 74.94346, 0.0}},
        {Rotator{-90.0, 45.0, 30.0}, Rotator{-90.0, 75.0, 0.0}},
        {Rotator{-90.0, 45.0, 0.0}, Rotator{-90.0, 45.0, 0.0}},
        {Rotator{-90.0, 0.0, 0.0}, Rotator{-90.0, 0.0, 0.0}},
    }));

    CHECK(Rotator{input.quaternion()}.equals(expected, NEAR_POLE_MARGIN));
}

TEST_CASE("Snapping to a pole stays within the orientation limit", "[Rotator]")
{
    const double pitch = GENERATE(-89.99, -89.95, -89.919, 89.919, 89.95, 89.99);
    const double yaw = GENERATE(-135.0, 45.0, 170.0);
    const double roll = GENERATE(-120.0, 0.0, 30.0);
    const glm::dquat rotation = Rotator{pitch, yaw, roll}.quaternion();

    const Rotator result{rotation};

    CHECK(std::abs(result.pitch()) == 90.0);
    CHECK(Orientation::differenceDegrees(rotation, result.quaternion()) < SNAPPED_ORIENTATION_LIMIT);
}

TEST_CASE("A Rotator exactly at a pole keeps its orientation", "[Rotator]")
{
    const double pitch = GENERATE(-90.0, 90.0);
    const double yaw = GENERATE(-135.0, 0.0, 45.0, 180.0);
    const double roll = GENERATE(-120.0, 0.0, 30.0);
    const glm::dquat rotation = Rotator{pitch, yaw, roll}.quaternion();

    const Rotator result{rotation};

    CHECK(result.pitch() == pitch);
    CHECK(Orientation::differenceDegrees(rotation, result.quaternion()) < ROUND_TRIP_TOLERANCE);
}

TEST_CASE("A quaternion and its negation give the same Rotator", "[Rotator]")
{
    const double pitch = GENERATE(-90.0, 40.0, 90.0);
    const glm::dquat rotation = Rotator{pitch, 25.0, 10.0}.quaternion();

    CHECK(Rotator{-rotation}.equals(Rotator{rotation}, ROUND_TRIP_TOLERANCE));
}

TEST_CASE("equals compares each axis modulo a full turn", "[Rotator]")
{
    CHECK(Rotator{10.0, 200.0, 0.0}.equals(Rotator{10.0, -160.0, 0.0}, ROUND_TRIP_TOLERANCE));
    CHECK(Rotator{0.0, 179.5, 0.0}.equals(Rotator{0.0, -179.5, 0.0}, 1.5));
}

TEST_CASE("equals includes the tolerance and checks every axis", "[Rotator]")
{
    const Rotator rotator{10.0, 20.0, 30.0};
    const auto [other, expected] = GENERATE_COPY(table<Rotator, bool>({
        {Rotator{10.5, 20.0, 30.0}, true},
        {Rotator{10.75, 20.0, 30.0}, false},
        {Rotator{10.0, 20.75, 30.0}, false},
        {Rotator{10.0, 20.0, 30.75}, false},
    }));

    CHECK(rotator.equals(other, 0.5) == expected);
}

TEST_CASE("operator== stays an exact comparison", "[Rotator]")
{
    CHECK(Rotator{10.0, 200.0, 0.0} != Rotator{10.0, -160.0, 0.0});
}

TEST_CASE_METHOD(DefaultLoggerFixture, "A non-unit quaternion reports a failed verify and still gives finite values", "[Rotator]")
{
    const auto [scale, expectedMessageCount] = GENERATE(table<double, std::size_t>({
        {1.0, 0u},
        {1.0 + 1e-8, 0u},
        {1.1, 1u},
        {0.5, 1u},
        {0.0, 1u},
    }));
    const glm::dquat rotation = Rotator{40.0, 25.0, 10.0}.quaternion() * scale;

    const Rotator result{rotation};

    CHECK(m_messageCount == expectedMessageCount);
    CHECK(std::isfinite(result.pitch()));
    CHECK(std::isfinite(result.yaw()));
    CHECK(std::isfinite(result.roll()));
}
} // namespace Hellfire::Engine::Math::RotatorTests
