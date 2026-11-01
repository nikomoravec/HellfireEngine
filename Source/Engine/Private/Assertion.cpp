#include "Assertion.hpp"
#include "Logger.hpp"

#include "fmt/format.h"

#include <cstdio>
#include <string>

#if __has_include(<stacktrace>)
#include <stacktrace>
#endif

namespace Hellfire::Engine
{
namespace
{
constexpr LogCategory HF_LOG_ASSERTION{"Assertion"};
} // namespace

void Assertion::writeFailure(std::string_view expression, const std::source_location& location, fmt::string_view format, fmt::format_args arguments) noexcept
{
    try
    {
        std::string text = fmt::format("verify condition '{}' failed at: {}:{}: {}", expression, location.file_name(), location.line(), fmt::vformat(format, arguments));
#if defined(__cpp_lib_stacktrace) && defined(HELLFIRE_DEBUG)
        text += fmt::format("\nstacktrace:\n{}", std::to_string(std::stacktrace::current()));
#endif

        const auto logger = Logger::defaultLogger();
        if (logger == nullptr)
        {
            fmt::println(stderr, "{}", text);
            return;
        }

        logger->error(HF_LOG_ASSERTION, "{}", text);
    }
    catch (...)
    {
    }
}
} // namespace Hellfire::Engine