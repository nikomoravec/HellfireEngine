#pragma once

#include "NoCopyNoMove.hpp"

#include "beman/cstring_view/cstring_view.hpp"
#include "fmt/base.h"
#include "fmt/os.h"

#include <chrono>
#include <cstdint>
#include <exception>
#include <expected>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Hellfire::Engine
{
enum class LogLevel : uint32_t
{
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class LogCategory final
{
public:
    constexpr explicit LogCategory(beman::cstring_view name, LogLevel minimumLevel = LogLevel::INFO) noexcept
        : m_name(name)
        , m_minimumLevel(minimumLevel)
    {
    }

    constexpr beman::cstring_view name() const noexcept
    {
        return m_name;
    }

    constexpr LogLevel minimumLevel() const noexcept
    {
        return m_minimumLevel;
    }

private:
    beman::cstring_view m_name;
    LogLevel m_minimumLevel;
};

class LogMessage final
{
public:
    constexpr LogMessage(const LogCategory& category, LogLevel level, std::chrono::system_clock::time_point time, std::string_view text) noexcept
        : m_category(category)
        , m_level(level)
        , m_time(time)
        , m_text(text)
    {
    }

    constexpr const LogCategory& category() const noexcept
    {
        return m_category;
    }

    constexpr LogLevel level() const noexcept
    {
        return m_level;
    }

    constexpr std::chrono::system_clock::time_point time() const noexcept
    {
        return m_time;
    }

    constexpr std::string_view text() const noexcept
    {
        return m_text;
    }

    std::string line() const;

private:
    LogCategory m_category;
    LogLevel m_level;
    std::chrono::system_clock::time_point m_time;
    std::string_view m_text;
};

class LogOutput : public NoCopyNoMove
{
public:
    virtual ~LogOutput() = default;

    virtual void write(const LogMessage& message) noexcept = 0;
};

class LogFileOutput final : public LogOutput
{
public:
    explicit LogFileOutput(fmt::buffered_file file) noexcept;
    void write(const LogMessage& message) noexcept override;

private:
    fmt::buffered_file m_file;
};

class LogConsoleOutput final : public LogOutput
{
public:
    void write(const LogMessage& message) noexcept override;
};

class Logger
{
public:
    Logger();

    void addOutput(std::unique_ptr<LogOutput> output);

    template<typename... Args>
    void debug(const LogCategory& category, fmt::format_string<Args...> format, Args&&... args) noexcept
    {
        write(category, LogLevel::DEBUG, format, fmt::make_format_args(args...));
    }

    template<typename... Args>
    void info(const LogCategory& category, fmt::format_string<Args...> format, Args&&... args) noexcept
    {
        write(category, LogLevel::INFO, format, fmt::make_format_args(args...));
    }

    template<typename... Args>
    void warning(const LogCategory& category, fmt::format_string<Args...> format, Args&&... args) noexcept
    {
        write(category, LogLevel::WARNING, format, fmt::make_format_args(args...));
    }

    template<typename... Args>
    void error(const LogCategory& category, fmt::format_string<Args...> format, Args&&... args) noexcept
    {
        write(category, LogLevel::ERROR, format, fmt::make_format_args(args...));
    }

    template<typename... Args>
    void fatal(const LogCategory& category, fmt::format_string<Args...> format, Args&&... args) noexcept
    {
        write(category, LogLevel::FATAL, format, fmt::make_format_args(args...));
        std::terminate();
    }

private:
    void write(const LogCategory& category, LogLevel level, fmt::string_view format, fmt::format_args arguments) noexcept;

private:
    mutable std::mutex m_mutex;
    std::vector<std::unique_ptr<LogOutput>> m_outputs;
};
} // namespace Hellfire::Engine

template<>
struct fmt::formatter<Hellfire::Engine::LogLevel> : fmt::formatter<fmt::string_view>
{
    auto format(Hellfire::Engine::LogLevel level, fmt::format_context& ctx) const
    {
        switch (level)
        {
        case Hellfire::Engine::LogLevel::DEBUG:
            return fmt::format_to(ctx.out(), "DEBUG");
        case Hellfire::Engine::LogLevel::INFO:
            return fmt::format_to(ctx.out(), "INFO");
        case Hellfire::Engine::LogLevel::WARNING:
            return fmt::format_to(ctx.out(), "WARNING");
        case Hellfire::Engine::LogLevel::ERROR:
            return fmt::format_to(ctx.out(), "ERROR");
        case Hellfire::Engine::LogLevel::FATAL:
            return fmt::format_to(ctx.out(), "FATAL");
        }

        return fmt::format_to(ctx.out(), "Unknown LogLevel ({})", std::to_underlying(level));
    }
};