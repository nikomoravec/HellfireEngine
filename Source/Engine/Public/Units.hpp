#pragma once

#include "Assertion.hpp"

#include "glm/glm.hpp"

#include <concepts>
#include <limits>
#include <type_traits>
#include <utility>

namespace Hellfire::Engine
{
inline constexpr double UNITS_PER_METER = 1.0;

template<typename T>
concept NumericInteger = std::integral<T> && !std::same_as<T, bool> && !std::same_as<T, char> && !std::same_as<T, wchar_t> && !std::same_as<T, char8_t> && !std::same_as<T, char16_t> && !std::same_as<T, char32_t>;
} // namespace Hellfire::Engine

template<std::floating_point T>
constexpr T HF_UNIT(T value) noexcept
{
    return static_cast<T>(value * ::Hellfire::Engine::UNITS_PER_METER);
}

template<::Hellfire::Engine::NumericInteger T>
constexpr T HF_UNIT(T value) noexcept
{
    constexpr double scale = ::Hellfire::Engine::UNITS_PER_METER;
    static_assert(scale >= 1.0 && scale <= static_cast<double>(std::numeric_limits<T>::max()) && static_cast<double>(static_cast<T>(scale)) == scale, "Integer HF_UNIT needs UNITS_PER_METER to be a whole number that fits the integer type");

    constexpr T integerScale = static_cast<T>(scale);
    HELLFIRE_VERIFY_RETURN_VALUE(std::cmp_less_equal(value, std::numeric_limits<T>::max() / integerScale), std::numeric_limits<T>::max(), "{} meters overflows the unit type", value);
    HELLFIRE_VERIFY_RETURN_VALUE(std::cmp_greater_equal(value, std::numeric_limits<T>::lowest() / integerScale), std::numeric_limits<T>::lowest(), "{} meters overflows the unit type", value);
    return static_cast<T>(value * integerScale);
}

template<glm::length_t L, std::floating_point T, glm::qualifier Q>
constexpr glm::vec<L, T, Q> HF_UNIT(const glm::vec<L, T, Q>& value) noexcept
{
    using Intermediate = std::common_type_t<T, double>;
    return glm::vec<L, T, Q>(glm::vec<L, Intermediate, Q>(value) * static_cast<Intermediate>(::Hellfire::Engine::UNITS_PER_METER));
}

template<glm::length_t L, ::Hellfire::Engine::NumericInteger T, glm::qualifier Q>
constexpr glm::vec<L, T, Q> HF_UNIT(const glm::vec<L, T, Q>& value) noexcept
{
    glm::vec<L, T, Q> result{value};
    for (glm::length_t index = 0; index < L; ++index)
    {
        result[index] = HF_UNIT(value[index]);
    }

    return result;
}
