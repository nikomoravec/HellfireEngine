#pragma once

#include "StaticOnly.hpp"

#include "fmt/base.h"

#include <exception>
#include <source_location>
#include <string_view>

namespace Hellfire::Engine
{
class Assertion : public StaticOnly
{
public:
    template<typename... Args>
    static void reportFailure(std::string_view expression, const std::source_location& location, fmt::format_string<Args...> format, Args&&... args) noexcept
    {
        writeFailure(expression, location, format, fmt::make_format_args(args...));
    }

private:
    static void writeFailure(std::string_view expression, const std::source_location& location, fmt::string_view format, fmt::format_args arguments) noexcept;
};
} // namespace Hellfire::Engine

#define HELLFIRE_DETAIL_REPORT_FAILURE(expressionText, format, ...) \
    ::Hellfire::Engine::Assertion::reportFailure(expressionText, std::source_location::current(), format __VA_OPT__(, ) __VA_ARGS__)

#define HELLFIRE_VERIFY_TERMINATE(expr, format, ...)                                  \
    do                                                                                \
    {                                                                                 \
        if (!(expr)) [[unlikely]]                                                     \
        {                                                                             \
            HELLFIRE_DETAIL_REPORT_FAILURE(#expr, format __VA_OPT__(, ) __VA_ARGS__); \
            std::terminate();                                                         \
        }                                                                             \
    } while (false)

#define HELLFIRE_VERIFY_THROW(expr, exception, ...)                        \
    do                                                                     \
    {                                                                      \
        if (!(expr)) [[unlikely]]                                          \
        {                                                                  \
            HELLFIRE_DETAIL_REPORT_FAILURE(#expr, "throwing " #exception); \
            throw exception{__VA_ARGS__};                                  \
        }                                                                  \
    } while (false)

#define HELLFIRE_VERIFY_LOG(expr, format, ...)                                        \
    do                                                                                \
    {                                                                                 \
        if (!(expr)) [[unlikely]]                                                     \
        {                                                                             \
            HELLFIRE_DETAIL_REPORT_FAILURE(#expr, format __VA_OPT__(, ) __VA_ARGS__); \
        }                                                                             \
    } while (false)

#define HELLFIRE_VERIFY_RETURN_VOID(expr, format, ...)                                \
    do                                                                                \
    {                                                                                 \
        if (!(expr)) [[unlikely]]                                                     \
        {                                                                             \
            HELLFIRE_DETAIL_REPORT_FAILURE(#expr, format __VA_OPT__(, ) __VA_ARGS__); \
            return;                                                                   \
        }                                                                             \
    } while (false)

#define HELLFIRE_VERIFY_RETURN_VALUE(expr, val, format, ...)                          \
    do                                                                                \
    {                                                                                 \
        if (!(expr)) [[unlikely]]                                                     \
        {                                                                             \
            HELLFIRE_DETAIL_REPORT_FAILURE(#expr, format __VA_OPT__(, ) __VA_ARGS__); \
            return val;                                                               \
        }                                                                             \
    } while (false)

#define HELLFIRE_VERIFY_CONTINUE(expr, format, ...)                               \
    if (!(expr)) [[unlikely]]                                                     \
    {                                                                             \
        HELLFIRE_DETAIL_REPORT_FAILURE(#expr, format __VA_OPT__(, ) __VA_ARGS__); \
        continue;                                                                 \
    }                                                                             \
    else                                                                          \
        ((void)0)

#define HELLFIRE_VERIFY_RETURN_UNEXPECTED(expr, error) \
    do                                                 \
    {                                                  \
        if (!(expr)) [[unlikely]]                      \
        {                                              \
            return std::unexpected(error);             \
        }                                              \
    } while (false)
