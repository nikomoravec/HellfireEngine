#pragma once

namespace Hellfire::Engine
{
struct NoCopy
{
    NoCopy() = default;

    NoCopy(const NoCopy&) = delete;
    NoCopy& operator=(const NoCopy&) = delete;

    NoCopy(NoCopy&&) = default;
    NoCopy& operator=(NoCopy&&) = default;

protected:
    ~NoCopy() = default;
};
} // namespace Hellfire::Engine
