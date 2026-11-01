#pragma once

namespace Hellfire::Engine
{
class StaticOnly
{
protected:
    StaticOnly() = delete;
    ~StaticOnly() = default;
};
} // namespace Hellfire::Engine
