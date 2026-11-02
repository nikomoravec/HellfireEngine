#pragma once

#include "StaticOnly.hpp"

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace Hellfire::Engine
{
class Hash final : public StaticOnly
{
public:
    [[nodiscard]] static constexpr uint64_t fnv1a(std::string_view str) noexcept
    {
        constexpr uint64_t FNV_OFFSET_BASIS = 0xCBF29CE484222325u;
        constexpr uint64_t FNV_PRIME = 0x100000001B3u;

        return std::ranges::fold_left(str, FNV_OFFSET_BASIS, [](uint64_t hash, char character) {
            return (hash ^ static_cast<uint64_t>(static_cast<uint8_t>(character))) * FNV_PRIME;
        });
    }
};
} // namespace Hellfire::Engine
