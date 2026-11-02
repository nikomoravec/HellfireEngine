#include "Logger.hpp"
#include "Units.hpp"

#include "catch2/catch_test_macros.hpp"
#include "glm/ext/vector_bool3.hpp"
#include "glm/ext/vector_double1.hpp"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/ext/vector_double4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int2.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/ext/vector_int4.hpp"
#include "glm/ext/vector_uint3.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>

namespace Hellfire::Engine::UnitsTests
{
constexpr std::int64_t INTEGER_SCALE = static_cast<std::int64_t>(UNITS_PER_METER);

template<typename T>
concept ConvertibleToUnits = requires(T value) { HF_UNIT(value); };

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

TEST_CASE("One meter is UNITS_PER_METER units", "[Units]")
{
    STATIC_CHECK(HF_UNIT(1.0f) == static_cast<float>(UNITS_PER_METER));
    STATIC_CHECK(HF_UNIT(1.0) == UNITS_PER_METER);
    STATIC_CHECK(HF_UNIT(1.0L) == static_cast<long double>(UNITS_PER_METER));
    STATIC_CHECK(HF_UNIT(1) == static_cast<int>(UNITS_PER_METER));
    STATIC_CHECK(HF_UNIT(1u) == static_cast<unsigned int>(UNITS_PER_METER));
}

TEST_CASE("HF_UNIT keeps zero and sign and scales linearly", "[Units]")
{
    STATIC_CHECK(HF_UNIT(0.0) == 0.0);
    STATIC_CHECK(HF_UNIT(-3.5) == -HF_UNIT(3.5));
    STATIC_CHECK(HF_UNIT(2.0) == 2.0 * HF_UNIT(1.0));
    STATIC_CHECK(HF_UNIT(0) == 0);
    STATIC_CHECK(HF_UNIT(-3) == -HF_UNIT(3));
    STATIC_CHECK(HF_UNIT(2) == 2 * HF_UNIT(1));
}

TEST_CASE("HF_UNIT returns the type it was given", "[Units]")
{
    STATIC_CHECK(std::same_as<decltype(HF_UNIT(1.0f)), float>);
    STATIC_CHECK(std::same_as<decltype(HF_UNIT(1.0)), double>);
    STATIC_CHECK(std::same_as<decltype(HF_UNIT(1.0L)), long double>);
    STATIC_CHECK(std::same_as<decltype(HF_UNIT(std::int8_t{1})), std::int8_t>);
    STATIC_CHECK(std::same_as<decltype(HF_UNIT(std::uint16_t{1})), std::uint16_t>);
    STATIC_CHECK(std::same_as<decltype(HF_UNIT(std::int64_t{1})), std::int64_t>);
    STATIC_CHECK(std::same_as<decltype(HF_UNIT(glm::vec3{})), glm::vec3>);
    STATIC_CHECK(std::same_as<decltype(HF_UNIT(glm::vec<3, float, glm::lowp>{})), glm::vec<3, float, glm::lowp>>);
    STATIC_CHECK(std::same_as<decltype(HF_UNIT(glm::ivec3{})), glm::ivec3>);
    STATIC_CHECK(std::same_as<decltype(HF_UNIT(glm::vec<3, int, glm::lowp>{})), glm::vec<3, int, glm::lowp>>);
}

TEST_CASE("HF_UNIT scales every component of a vector", "[Units]")
{
    STATIC_CHECK(HF_UNIT(glm::dvec1{-2.0}) == glm::dvec1{HF_UNIT(-2.0)});
    STATIC_CHECK(HF_UNIT(glm::dvec2{1.0, -2.0}) == glm::dvec2{HF_UNIT(1.0), HF_UNIT(-2.0)});
    STATIC_CHECK(HF_UNIT(glm::dvec3{1.0, -2.0, 3.5}) == glm::dvec3{HF_UNIT(1.0), HF_UNIT(-2.0), HF_UNIT(3.5)});
    STATIC_CHECK(HF_UNIT(glm::dvec4{1.0, -2.0, 3.5, 0.0}) == glm::dvec4{HF_UNIT(1.0), HF_UNIT(-2.0), HF_UNIT(3.5), HF_UNIT(0.0)});
    STATIC_CHECK(HF_UNIT(glm::ivec2{1, -2}) == glm::ivec2{HF_UNIT(1), HF_UNIT(-2)});
    STATIC_CHECK(HF_UNIT(glm::ivec3{1, -2, 3}) == glm::ivec3{HF_UNIT(1), HF_UNIT(-2), HF_UNIT(3)});
    STATIC_CHECK(HF_UNIT(glm::ivec4{1, -2, 3, 0}) == glm::ivec4{HF_UNIT(1), HF_UNIT(-2), HF_UNIT(3), HF_UNIT(0)});
    STATIC_CHECK(HF_UNIT(glm::uvec3{1u, 2u, 3u}) == glm::uvec3{HF_UNIT(1u), HF_UNIT(2u), HF_UNIT(3u)});
}

TEST_CASE("A vector rounds each component like the scalar overload", "[Units]")
{
    // Not representable as double where long double is wider, so narrowing through double would change it.
    constexpr long double wideValue = 1.0L + std::numeric_limits<long double>::epsilon();

    STATIC_CHECK(HF_UNIT(glm::vec3{0.1f, 1.0f / 3.0f, -7.3f}) == glm::vec3{HF_UNIT(0.1f), HF_UNIT(1.0f / 3.0f), HF_UNIT(-7.3f)});
    STATIC_CHECK(HF_UNIT(glm::vec<1, long double>{wideValue}) == glm::vec<1, long double>{HF_UNIT(wideValue)});
}

TEST_CASE("Integer HF_UNIT stays exact up to the type limits", "[Units]")
{
    constexpr std::int64_t largest = std::numeric_limits<std::int64_t>::max() / INTEGER_SCALE;
    constexpr std::int64_t smallest = std::numeric_limits<std::int64_t>::lowest() / INTEGER_SCALE;
    constexpr std::uint64_t largestUnsigned = std::numeric_limits<std::uint64_t>::max() / static_cast<std::uint64_t>(INTEGER_SCALE);
    // Above 2^53, where a round trip through double would lose the last bit.
    constexpr std::int64_t beyondDoublePrecision = (std::int64_t{1} << 53) + 1;

    STATIC_CHECK(HF_UNIT(largest) == largest * INTEGER_SCALE);
    STATIC_CHECK(HF_UNIT(smallest) == smallest * INTEGER_SCALE);
    STATIC_CHECK(HF_UNIT(largestUnsigned) == largestUnsigned * static_cast<std::uint64_t>(INTEGER_SCALE));
    STATIC_CHECK(HF_UNIT(beyondDoublePrecision) == beyondDoublePrecision * INTEGER_SCALE);
    STATIC_CHECK(HF_UNIT(glm::vec<3, std::int64_t>{largest, smallest, beyondDoublePrecision}) == glm::vec<3, std::int64_t>{largest * INTEGER_SCALE, smallest * INTEGER_SCALE, beyondDoublePrecision * INTEGER_SCALE});
}

TEST_CASE_METHOD(DefaultLoggerFixture, "Integer HF_UNIT reports overflow and clamps to the type limits", "[Units]")
{
    if (INTEGER_SCALE == 1)
    {
        SKIP("No integer value can overflow while UNITS_PER_METER is 1");
    }

    CHECK(HF_UNIT(std::numeric_limits<std::int64_t>::max()) == std::numeric_limits<std::int64_t>::max());
    CHECK(HF_UNIT(std::numeric_limits<std::int64_t>::lowest()) == std::numeric_limits<std::int64_t>::lowest());
    CHECK(HF_UNIT(std::numeric_limits<std::uint64_t>::max()) == std::numeric_limits<std::uint64_t>::max());
    CHECK(HF_UNIT(std::numeric_limits<std::int8_t>::max()) == std::numeric_limits<std::int8_t>::max());
    CHECK(HF_UNIT(std::numeric_limits<std::int8_t>::lowest()) == std::numeric_limits<std::int8_t>::lowest());
    CHECK(m_messageCount == 5u);
}

TEST_CASE_METHOD(DefaultLoggerFixture, "Integer HF_UNIT does not report values at the overflow boundary", "[Units]")
{
    constexpr std::int64_t largest = std::numeric_limits<std::int64_t>::max() / INTEGER_SCALE;
    constexpr std::int64_t smallest = std::numeric_limits<std::int64_t>::lowest() / INTEGER_SCALE;

    CHECK(HF_UNIT(largest) == largest * INTEGER_SCALE);
    CHECK(HF_UNIT(smallest) == smallest * INTEGER_SCALE);
    CHECK(m_messageCount == 0u);
}

TEST_CASE_METHOD(DefaultLoggerFixture, "An integer vector clamps only the components that overflow", "[Units]")
{
    if (INTEGER_SCALE == 1)
    {
        SKIP("No integer value can overflow while UNITS_PER_METER is 1");
    }

    const glm::ivec3 result = HF_UNIT(glm::ivec3{1, std::numeric_limits<int>::max(), std::numeric_limits<int>::lowest()});

    CHECK(result == glm::ivec3{HF_UNIT(1), std::numeric_limits<int>::max(), std::numeric_limits<int>::lowest()});
    CHECK(m_messageCount == 2u);
}

TEST_CASE("HF_UNIT accepts floating point and integer scalars and vectors", "[Units]")
{
    STATIC_CHECK(ConvertibleToUnits<float>);
    STATIC_CHECK(ConvertibleToUnits<double>);
    STATIC_CHECK(ConvertibleToUnits<long double>);
    STATIC_CHECK(ConvertibleToUnits<int>);
    STATIC_CHECK(ConvertibleToUnits<unsigned int>);
    STATIC_CHECK(ConvertibleToUnits<std::int8_t>);
    STATIC_CHECK(ConvertibleToUnits<std::uint64_t>);
    STATIC_CHECK(ConvertibleToUnits<glm::vec3>);
    STATIC_CHECK(ConvertibleToUnits<glm::dvec3>);
    STATIC_CHECK(ConvertibleToUnits<glm::ivec3>);
    STATIC_CHECK(ConvertibleToUnits<glm::uvec3>);
}

TEST_CASE("HF_UNIT rejects bool and character types", "[Units]")
{
    STATIC_CHECK_FALSE(ConvertibleToUnits<bool>);
    STATIC_CHECK_FALSE(ConvertibleToUnits<char>);
    STATIC_CHECK_FALSE(ConvertibleToUnits<char8_t>);
    STATIC_CHECK_FALSE(ConvertibleToUnits<char16_t>);
    STATIC_CHECK_FALSE(ConvertibleToUnits<char32_t>);
    STATIC_CHECK_FALSE(ConvertibleToUnits<wchar_t>);
    STATIC_CHECK_FALSE(ConvertibleToUnits<glm::bvec3>);
}
} // namespace Hellfire::Engine::UnitsTests
