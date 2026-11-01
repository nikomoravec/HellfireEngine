#include "Logger.hpp"
#include "NoCopyNoMove.hpp"
#include "StaticOnly.hpp"

#include "catch2/catch_test_macros.hpp"
#include "catch2/generators/catch_generators.hpp"
#include "catch2/matchers/catch_matchers_string.hpp"
#include "fmt/format.h"
#include "fmt/os.h"
#include "fmt/std.h"

#ifdef _WIN32
#include <process.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace Hellfire::Engine::LoggerTests
{
class FormatCounter final
{
public:
    explicit FormatCounter(int& count) noexcept
        : m_count(count)
    {
    }

    void increment() const noexcept
    {
        ++m_count;
    }

private:
    int& m_count;
};
} // namespace Hellfire::Engine::LoggerTests

template<>
struct fmt::formatter<Hellfire::Engine::LoggerTests::FormatCounter> : fmt::formatter<fmt::string_view>
{
    auto format(const Hellfire::Engine::LoggerTests::FormatCounter& counter, fmt::format_context& context) const
    {
        counter.increment();
        return context.out();
    }
};

namespace Hellfire::Engine::LoggerTests
{
using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::EndsWith;
using Catch::Matchers::StartsWith;

constexpr LogCategory TEST_CATEGORY{"Tests"};
constexpr auto FATAL_PROCESS_TAG = "[.fatalProcess]";
constexpr int FATAL_EXIT_CODE = 42;

class FixedLocalTime final : public StaticOnly
{
public:
    static constexpr std::string_view FORMATTED = "12:34:56.789";

    // Noon in mid-January is never inside a daylight saving transition, so it exists exactly once in every zone.
    static std::chrono::system_clock::time_point value()
    {
        using namespace std::chrono;

        const auto localTime = local_days{2026y / January / 15d} + 12h + 34min + 56s + 789ms;
        return current_zone()->to_sys(localTime);
    }
};

class TextFile final : public StaticOnly
{
public:
    static std::string read(const std::filesystem::path& path)
    {
        std::ifstream file{path};
        return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    }
};

class TemporaryDirectory final : public NoCopyNoMove
{
public:
    TemporaryDirectory()
        : m_path(std::filesystem::temp_directory_path() / fmt::format("HellfireLoggerTests-{}", std::random_device{}()))
    {
        std::filesystem::create_directories(m_path);
    }

    ~TemporaryDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(m_path, error);
    }

    const std::filesystem::path& path() const noexcept
    {
        return m_path;
    }

private:
    std::filesystem::path m_path;
};

class StreamRedirect final : public NoCopyNoMove
{
public:
    StreamRedirect(std::FILE* stream, const std::filesystem::path& target, int openFlags)
        : m_stream(stream)
        , m_original(fmt::file::dup(fileno(stream)))
    {
        std::fflush(m_stream);
        fmt::file{target.string(), openFlags}.dup2(fileno(m_stream));
    }

    ~StreamRedirect()
    {
        std::fflush(m_stream);
        std::clearerr(m_stream);
        std::error_code error;
        m_original.dup2(fileno(m_stream), error);
    }

private:
    std::FILE* m_stream;
    fmt::file m_original;
};

class CapturedConsole final
{
public:
    CapturedConsole(std::string standardOutput, std::string standardError) noexcept
        : m_standardOutput(std::move(standardOutput))
        , m_standardError(std::move(standardError))
    {
    }

    template<std::invocable Action>
    static CapturedConsole during(Action action)
    {
        const TemporaryDirectory directory;
        const auto standardOutputPath = directory.path() / "stdout.txt";
        const auto standardErrorPath = directory.path() / "stderr.txt";

        {
            constexpr int OPEN_FLAGS = fmt::file::WRONLY | fmt::file::CREATE | fmt::file::TRUNC;
            const StreamRedirect standardOutputRedirect{stdout, standardOutputPath, OPEN_FLAGS};
            const StreamRedirect standardErrorRedirect{stderr, standardErrorPath, OPEN_FLAGS};
            action();
        }

        return {TextFile::read(standardOutputPath), TextFile::read(standardErrorPath)};
    }

    const std::string& standardOutput() const noexcept
    {
        return m_standardOutput;
    }

    const std::string& standardError() const noexcept
    {
        return m_standardError;
    }

private:
    std::string m_standardOutput;
    std::string m_standardError;
};

class FatalProcess final : public StaticOnly
{
public:
    // The terminate handler exits instead of aborting so gcov can flush its counters.
    static void logFatalMessage()
    {
        std::set_terminate([] { std::exit(FATAL_EXIT_CODE); });
        Logger logger;
        logger.addOutput(std::make_unique<LogConsoleOutput>());

        logger.fatal(TEST_CATEGORY, "fatal {}", 7);
    }

    static int run()
    {
#ifdef _WIN32
        const auto quotedProgramPath = fmt::format("\"{}\"", _pgmptr);
        return static_cast<int>(_spawnl(_P_WAIT, _pgmptr, quotedProgramPath.c_str(), FATAL_PROCESS_TAG, nullptr));
#else
        const pid_t processId = fork();
        if (processId == 0)
        {
            logFatalMessage();
            _exit(EXIT_SUCCESS);
        }

        int status = 0;
        waitpid(processId, &status, 0);
        return WEXITSTATUS(status);
#endif
    }
};

class RecordedMessage final
{
public:
    explicit RecordedMessage(const LogMessage& message)
        : m_categoryName(message.category().name().c_str())
        , m_level(message.level())
        , m_time(message.time())
        , m_text(message.text())
    {
    }

    const std::string& categoryName() const noexcept
    {
        return m_categoryName;
    }

    LogLevel level() const noexcept
    {
        return m_level;
    }

    std::chrono::system_clock::time_point time() const noexcept
    {
        return m_time;
    }

    const std::string& text() const noexcept
    {
        return m_text;
    }

private:
    std::string m_categoryName;
    LogLevel m_level;
    std::chrono::system_clock::time_point m_time;
    std::string m_text;
};

class RecordingOutput final : public LogOutput
{
public:
    explicit RecordingOutput(std::vector<RecordedMessage>& messages) noexcept
        : m_messages(messages)
    {
    }

    void write(const LogMessage& message) noexcept override
    {
        m_messages.emplace_back(message);
    }

private:
    std::vector<RecordedMessage>& m_messages;
};

class DestructionFlagOutput final : public LogOutput
{
public:
    explicit DestructionFlagOutput(bool& destroyed) noexcept
        : m_destroyed(destroyed)
    {
    }

    ~DestructionFlagOutput() override
    {
        m_destroyed = true;
    }

    void write(const LogMessage&) noexcept override
    {
    }

private:
    bool& m_destroyed;
};

class RecordingLoggerFixture
{
public:
    RecordingLoggerFixture()
        : m_messages()
        , m_logger()
    {
        m_logger.addOutput(std::make_unique<RecordingOutput>(m_messages));
    }

protected:
    std::vector<RecordedMessage> m_messages;
    Logger m_logger;
};

TEST_CASE("LogLevel formats as its name", "[Logger][LogLevel]")
{
    const auto [level, expectedName] = GENERATE(table<LogLevel, std::string_view>({
        {LogLevel::DEBUG, "DEBUG"},
        {LogLevel::INFO, "INFO"},
        {LogLevel::WARNING, "WARNING"},
        {LogLevel::ERROR, "ERROR"},
        {LogLevel::FATAL, "FATAL"},
    }));

    CHECK(fmt::format("{}", level) == expectedName);
}

TEST_CASE("LogLevel formats an unknown value with its number", "[Logger][LogLevel]")
{
    CHECK(fmt::format("{}", static_cast<LogLevel>(99)) == "Unknown LogLevel (99)");
}

TEST_CASE("LogCategory stores its name and minimum level", "[Logger][LogCategory]")
{
    const auto minimumLevel = GENERATE(LogLevel::DEBUG, LogLevel::FATAL);

    const LogCategory category{"Renderer", minimumLevel};

    CHECK(std::string_view{category.name().c_str()} == "Renderer");
    CHECK(category.minimumLevel() == minimumLevel);
}

TEST_CASE("LogCategory defaults its minimum level to INFO", "[Logger][LogCategory]")
{
    STATIC_REQUIRE(LogCategory{"Renderer"}.minimumLevel() == LogLevel::INFO);
}

TEST_CASE("LogMessage stores its fields", "[Logger][LogMessage]")
{
    const LogCategory category{"Renderer", LogLevel::WARNING};
    const auto time = std::chrono::system_clock::now();

    const LogMessage message{category, LogLevel::ERROR, time, "text"};

    CHECK(std::string_view{message.category().name().c_str()} == "Renderer");
    CHECK(message.category().minimumLevel() == LogLevel::WARNING);
    CHECK(message.level() == LogLevel::ERROR);
    CHECK(message.time() == time);
    CHECK(message.text() == "text");
}

TEST_CASE("LogMessage line contains the thread time level category and text", "[Logger][LogMessage]")
{
    const LogCategory category{"Renderer"};
    const LogMessage message{category, LogLevel::WARNING, FixedLocalTime::value(), "frame dropped"};

    const auto expectedLine = fmt::format("[thread:{}][{}][WARNING][Renderer]: frame dropped", std::this_thread::get_id(), FixedLocalTime::FORMATTED);

    CHECK(message.line() == expectedLine);
}

TEST_CASE("LogMessage line truncates the time to milliseconds", "[Logger][LogMessage]")
{
    using namespace std::chrono_literals;

    const LogMessage message{TEST_CATEGORY, LogLevel::INFO, FixedLocalTime::value() + 999us, "text"};

    CHECK_THAT(message.line(), ContainsSubstring(fmt::format("[{}]", FixedLocalTime::FORMATTED)));
}

TEST_CASE("LogMessage line reports the calling thread", "[Logger][LogMessage]")
{
    const LogMessage message{TEST_CATEGORY, LogLevel::INFO, FixedLocalTime::value(), "text"};
    std::string line;
    std::string expectedPrefix;

    std::thread thread{[&] {
        line = message.line();
        expectedPrefix = fmt::format("[thread:{}]", std::this_thread::get_id());
    }};
    thread.join();

    CHECK_THAT(line, StartsWith(expectedPrefix));
}

TEST_CASE("LogConsoleOutput writes messages below ERROR to stdout", "[Logger][LogConsoleOutput]")
{
    const auto level = GENERATE(LogLevel::DEBUG, LogLevel::INFO, LogLevel::WARNING);
    const LogMessage message{TEST_CATEGORY, level, FixedLocalTime::value(), "text"};
    LogConsoleOutput output;

    const auto console = CapturedConsole::during([&] { output.write(message); });

    CHECK(console.standardOutput() == message.line() + "\n");
    CHECK(console.standardError().empty());
}

TEST_CASE("LogConsoleOutput writes ERROR and FATAL messages to stderr", "[Logger][LogConsoleOutput]")
{
    const auto level = GENERATE(LogLevel::ERROR, LogLevel::FATAL);
    const LogMessage message{TEST_CATEGORY, level, FixedLocalTime::value(), "text"};
    LogConsoleOutput output;

    const auto console = CapturedConsole::during([&] { output.write(message); });

    CHECK(console.standardOutput().empty());
    CHECK(console.standardError() == message.line() + "\n");
}

TEST_CASE("LogConsoleOutput survives a stream that rejects writes", "[Logger][LogConsoleOutput]")
{
    const auto [level, stream] = GENERATE(table<LogLevel, std::FILE*>({
        {LogLevel::INFO, stdout},
        {LogLevel::ERROR, stderr},
    }));
    const TemporaryDirectory directory;
    const auto path = directory.path() / "ReadOnly.txt";
    // Larger than the stream buffer, so the write fails right away instead of on a later flush.
    const std::string text(1'000'000, 'x');
    const LogMessage message{TEST_CATEGORY, level, FixedLocalTime::value(), text};
    LogConsoleOutput output;

    {
        const StreamRedirect redirect{stream, path, fmt::file::RDONLY | fmt::file::CREATE};
        output.write(message);
    }

    CHECK(TextFile::read(path).empty());
}

TEST_CASE("LogFileOutput flushes every line to the file as it is written", "[Logger][LogFileOutput]")
{
    const TemporaryDirectory directory;
    const auto path = directory.path() / "Log.txt";
    const LogMessage firstMessage{TEST_CATEGORY, LogLevel::INFO, FixedLocalTime::value(), "first"};
    const LogMessage secondMessage{TEST_CATEGORY, LogLevel::ERROR, FixedLocalTime::value(), "second"};
    LogFileOutput output{fmt::buffered_file{path.string(), "w"}};

    output.write(firstMessage);
    CHECK(TextFile::read(path) == firstMessage.line() + "\n");

    output.write(secondMessage);
    CHECK(TextFile::read(path) == firstMessage.line() + "\n" + secondMessage.line() + "\n");
}

TEST_CASE("LogFileOutput survives a file that rejects writes", "[Logger][LogFileOutput]")
{
    const TemporaryDirectory directory;
    const auto path = directory.path() / "ReadOnly.txt";
    const LogMessage message{TEST_CATEGORY, LogLevel::INFO, FixedLocalTime::value(), "text"};
    fmt::buffered_file{path.string(), "w"}.close();
    fmt::buffered_file file{path.string(), "r"};
    // fmt writes to a buffered glibc FILE directly and never reports the failure, an unbuffered one makes print throw everywhere.
    std::setvbuf(file.get(), nullptr, _IONBF, 0);

    {
        LogFileOutput output{std::move(file)};
        output.write(message);
    }

    CHECK(TextFile::read(path).empty());
}

TEST_CASE("LogFileOutput survives a failed flush", "[Logger][LogFileOutput]")
{
    const TemporaryDirectory directory;
    const auto path = directory.path() / "Log.txt";
    const auto readOnlyPath = directory.path() / "ReadOnly.txt";
    const LogMessage message{TEST_CATEGORY, LogLevel::INFO, FixedLocalTime::value(), "text"};
    fmt::buffered_file file{path.string(), "w"};
    fmt::file{readOnlyPath.string(), fmt::file::RDONLY | fmt::file::CREATE}.dup2(file.descriptor());

    {
        LogFileOutput output{std::move(file)};
        output.write(message);
    }

    CHECK(TextFile::read(path).empty());
    CHECK(TextFile::read(readOnlyPath).empty());
}

TEST_CASE_METHOD(RecordingLoggerFixture, "Logger forwards every severity to its output", "[Logger]")
{
    const LogCategory category{"Verbose", LogLevel::DEBUG};

    m_logger.debug(category, "debug {}", 1);
    m_logger.info(category, "info {}", 2);
    m_logger.warning(category, "warning {}", 3);
    m_logger.error(category, "error {}", 4);

    REQUIRE(m_messages.size() == 4);
    CHECK(m_messages[0].level() == LogLevel::DEBUG);
    CHECK(m_messages[0].text() == "debug 1");
    CHECK(m_messages[1].level() == LogLevel::INFO);
    CHECK(m_messages[1].text() == "info 2");
    CHECK(m_messages[2].level() == LogLevel::WARNING);
    CHECK(m_messages[2].text() == "warning 3");
    CHECK(m_messages[3].level() == LogLevel::ERROR);
    CHECK(m_messages[3].text() == "error 4");
    CHECK(m_messages[3].categoryName() == "Verbose");
}

TEST_CASE_METHOD(RecordingLoggerFixture, "Logger drops messages below the category minimum level", "[Logger]")
{
    const auto [minimumLevel, expectedMessageCount] = GENERATE(table<LogLevel, std::size_t>({
        {LogLevel::DEBUG, 4u},
        {LogLevel::INFO, 3u},
        {LogLevel::WARNING, 2u},
        {LogLevel::ERROR, 1u},
        {LogLevel::FATAL, 0u},
    }));
    const LogCategory category{"Filtered", minimumLevel};

    m_logger.debug(category, "message");
    m_logger.info(category, "message");
    m_logger.warning(category, "message");
    m_logger.error(category, "message");

    CHECK(m_messages.size() == expectedMessageCount);
}

TEST_CASE_METHOD(RecordingLoggerFixture, "Logger formats arguments only for messages that pass the filter", "[Logger]")
{
    int formatCount = 0;
    const FormatCounter counter{formatCount};

    m_logger.debug(TEST_CATEGORY, "{}", counter);
    CHECK(formatCount == 0);

    m_logger.info(TEST_CATEGORY, "{}", counter);
    CHECK(formatCount == 1);
}

TEST_CASE_METHOD(RecordingLoggerFixture, "Logger accepts lvalue and rvalue arguments", "[Logger]")
{
    int mutableValue = 1;
    const std::string constantValue = "two";

    m_logger.info(TEST_CATEGORY, "{} {} {}", mutableValue, constantValue, 3.5);

    REQUIRE(m_messages.size() == 1);
    CHECK(m_messages[0].text() == "1 two 3.5");
}

TEST_CASE_METHOD(RecordingLoggerFixture, "Logger stamps messages with the current time", "[Logger]")
{
    const auto earliestTime = std::chrono::system_clock::now();

    m_logger.info(TEST_CATEGORY, "text");

    const auto latestTime = std::chrono::system_clock::now();
    REQUIRE(m_messages.size() == 1);
    CHECK(m_messages[0].time() >= earliestTime);
    CHECK(m_messages[0].time() <= latestTime);
}

TEST_CASE_METHOD(RecordingLoggerFixture, "Logger writes every message to every output", "[Logger]")
{
    std::vector<RecordedMessage> secondMessages;
    m_logger.addOutput(std::make_unique<RecordingOutput>(secondMessages));

    m_logger.info(TEST_CATEGORY, "text");

    REQUIRE(m_messages.size() == 1);
    REQUIRE(secondMessages.size() == 1);
    CHECK(m_messages[0].text() == "text");
    CHECK(secondMessages[0].text() == "text");
}

TEST_CASE("Logger without outputs accepts messages", "[Logger]")
{
    Logger logger;

    logger.info(TEST_CATEGORY, "text");

    SUCCEED();
}

TEST_CASE_METHOD(RecordingLoggerFixture, "Logger drops a message with an invalid format and keeps working", "[Logger]")
{
    m_logger.info(TEST_CATEGORY, fmt::runtime("{} {}"), 1);
    CHECK(m_messages.empty());

    m_logger.info(TEST_CATEGORY, "recovered");

    REQUIRE(m_messages.size() == 1);
    CHECK(m_messages[0].text() == "recovered");
}

TEST_CASE("Logger destroys its outputs", "[Logger]")
{
    bool destroyed = false;

    {
        Logger logger;
        logger.addOutput(std::make_unique<DestructionFlagOutput>(destroyed));
        CHECK_FALSE(destroyed);
    }

    CHECK(destroyed);
}

TEST_CASE_METHOD(RecordingLoggerFixture, "Logger serializes writes from concurrent threads", "[Logger]")
{
    constexpr std::size_t THREAD_COUNT = 8;
    constexpr std::size_t MESSAGES_PER_THREAD = 250;

    std::vector<std::thread> threads;
    for (std::size_t threadIndex = 0; threadIndex < THREAD_COUNT; ++threadIndex)
    {
        threads.emplace_back([this] {
            for (std::size_t messageIndex = 0; messageIndex < MESSAGES_PER_THREAD; ++messageIndex)
            {
                m_logger.info(TEST_CATEGORY, "message {}", messageIndex);
            }
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    CHECK(m_messages.size() == THREAD_COUNT * MESSAGES_PER_THREAD);
}

TEST_CASE("Logger fatal writes the message and terminates the process", "[Logger]")
{
    int exitCode = 0;

    const auto console = CapturedConsole::during([&exitCode] { exitCode = FatalProcess::run(); });

    CHECK(exitCode == FATAL_EXIT_CODE);
    CHECK_THAT(console.standardError(), EndsWith("[FATAL][Tests]: fatal 7\n"));
}

TEST_CASE("Logger fatal child process", FATAL_PROCESS_TAG)
{
    FatalProcess::logFatalMessage();
}
} // namespace Hellfire::Engine::LoggerTests
