#include "Logger.hpp"

#include "beman/cstring_view/cstring_view.hpp"
#include "fmt/base.h"
#include "fmt/chrono.h"
#include "fmt/format.h"
#include "fmt/std.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>

namespace Hellfire::Engine
{
namespace
{
constinit std::atomic<Logger*> g_defaultLogger{nullptr};
} // namespace

std::string LogMessage::line() const
{
    const auto localTime = std::chrono::current_zone()->to_local(std::chrono::floor<std::chrono::milliseconds>(m_time));
    const auto threadId = std::this_thread::get_id();

    return fmt::format("[thread:{}][{:%H:%M:%S}][{}][{}]: {}", threadId, localTime, m_level, m_category.name().c_str(), m_text);
}

void LogConsoleOutput::write(const LogMessage& message) noexcept
{
    try
    {
        const auto logLevel = message.level();
        if (logLevel == LogLevel::ERROR || logLevel == LogLevel::FATAL)
        {
            fmt::println(stderr, "{}", message.line());
            return;
        }

        fmt::println(stdout, "{}", message.line());
    }
    catch (...)
    {
    }
}

LogFileOutput::LogFileOutput(fmt::buffered_file file) noexcept
    : m_file(std::move(file))
{
}

void LogFileOutput::write(const LogMessage& message) noexcept
{
    try
    {
        m_file.print("{}\n", message.line());
        std::fflush(m_file.get());
    }
    catch (...)
    {
    }
}

Logger::Logger()
    : m_mutex()
    , m_outputs()
{
}

Logger::~Logger()
{
    Logger* expected = this;
    g_defaultLogger.compare_exchange_strong(expected, nullptr);
}

void Logger::addOutput(std::unique_ptr<LogOutput> output)
{
    const std::scoped_lock lock{m_mutex};
    m_outputs.push_back(std::move(output));
}

void Logger::makeDefault() noexcept
{
    g_defaultLogger.store(this);
}

Logger* Logger::defaultLogger() noexcept
{
    return g_defaultLogger.load();
}

void Logger::write(const LogCategory& category, LogLevel level, fmt::string_view format, fmt::format_args arguments) noexcept
{
    if (level < category.minimumLevel())
    {
        return;
    }

    try
    {
        const std::string text = fmt::vformat(format, arguments);
        const LogMessage message{category, level, std::chrono::system_clock::now(), text};

        const std::scoped_lock lock{m_mutex};
        for (auto&& output : m_outputs)
        {
            output->write(message);
        }
    }
    catch (...)
    {
    }
}
} // namespace Hellfire::Engine
