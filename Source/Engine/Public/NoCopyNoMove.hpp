#pragma once

namespace Hellfire::Engine
{
struct NoCopyNoMove
{
    NoCopyNoMove() = default;

    NoCopyNoMove(const NoCopyNoMove&) = delete;
    NoCopyNoMove& operator=(const NoCopyNoMove&) = delete;

    NoCopyNoMove(NoCopyNoMove&&) = delete;
    NoCopyNoMove& operator=(NoCopyNoMove&&) = delete;

protected:
    ~NoCopyNoMove() = default;
};
} // namespace Hellfire::Engine
