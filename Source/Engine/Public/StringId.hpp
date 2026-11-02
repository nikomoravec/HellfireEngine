#pragma once

#include "Hash.hpp"

#include <cstdint>
#include <string_view>

namespace Hellfire::Engine
{
class StringId final
{
public:
    using ValueType = uint64_t;

private:
    static constexpr ValueType EMPTY_HASH = Hash::fnv1a("");

public:
    constexpr StringId() noexcept
        : m_id(EMPTY_HASH)
    {
    }

    constexpr explicit StringId(std::string_view str) noexcept
        : m_id(Hash::fnv1a(str))
    {
    }

    constexpr bool isEmpty() const noexcept
    {
        return m_id == EMPTY_HASH;
    }

    constexpr ValueType value() const noexcept
    {
        return m_id;
    }

    constexpr auto operator<=>(const StringId& other) const noexcept = default;

private:
    ValueType m_id;
};
} // namespace Hellfire::Engine

template<>
struct std::hash<Hellfire::Engine::StringId>
{
    size_t operator()(const Hellfire::Engine::StringId& id) const noexcept
    {
        return static_cast<size_t>(id.value());
    }
};
