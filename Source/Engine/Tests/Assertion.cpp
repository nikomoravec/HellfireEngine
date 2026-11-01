#include "Assertion.hpp"
#include "Logger.hpp"
#include "NoCopyNoMove.hpp"
#include "StaticOnly.hpp"

#include "catch2/catch_test_macros.hpp"
#include "catch2/generators/catch_generators.hpp"
#include "catch2/matchers/catch_matchers_string.hpp"
#include "fmt/format.h"
#include "fmt/os.h"

#ifdef _WIN32
#include <process.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <new>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>
#include <version>

#ifdef __cpp_lib_expected
#include <expected>
#endif

#define HELLFIRE_TESTS_FALSE_CONDITION false

namespace Hellfire::Engine::AssertionTests
{
class UnformattableValue final
{
};
} // namespace Hellfire::Engine::AssertionTests

template<>
struct fmt::formatter<Hellfire::Engine::AssertionTests::UnformattableValue> : fmt::formatter<fmt::string_view>
{
    fmt::format_context::iterator format(const Hellfire::Engine::AssertionTests::UnformattableValue&, fmt::format_context&) const
    {
        throw std::runtime_error{"value cannot be formatted"};
    }
};

namespace Hellfire::Engine::AssertionTests
{
using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::EndsWith;
using Catch::Matchers::StartsWith;

constexpr auto TERMINATE_PROCESS_TAG = "[.assertionTerminateProcess]";
constexpr int TERMINATE_EXIT_CODE = 43;

class CapturedStream final : public NoCopyNoMove
{
public:
    explicit CapturedStream(std::FILE* stream, int openFlags = fmt::file::WRONLY | fmt::file::CREATE | fmt::file::TRUNC)
        : m_stream(stream)
        , m_original(fmt::file::dup(fileno(stream)))
        , m_path(std::filesystem::temp_directory_path() / fmt::format("HellfireAssertionTests-{}.txt", std::random_device{}()))
    {
        std::fflush(m_stream);
        fmt::file{m_path.string(), openFlags}.dup2(fileno(m_stream));
    }

    ~CapturedStream()
    {
        std::fflush(m_stream);
        std::clearerr(m_stream);
        std::error_code error;
        m_original.dup2(fileno(m_stream), error);
        std::filesystem::remove(m_path, error);
    }

    std::string text() const
    {
        std::fflush(m_stream);
        std::ifstream file{m_path};
        return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    }

private:
    std::FILE* m_stream;
    fmt::file m_original;
    std::filesystem::path m_path;
};

class TerminatingProcess final : public StaticOnly
{
public:
    static void verify(bool condition)
    {
        HELLFIRE_VERIFY_TERMINATE(condition, "fatal {}", 7);
    }

    // The terminate handler exits instead of aborting so gcov can flush its counters.
    static void failVerification()
    {
        std::set_terminate([] { std::exit(TERMINATE_EXIT_CODE); });
        verify(false);
    }

    static int run()
    {
#ifdef _WIN32
        const auto quotedProgramPath = fmt::format("\"{}\"", _pgmptr);
        return static_cast<int>(_spawnl(_P_WAIT, _pgmptr, quotedProgramPath.c_str(), TERMINATE_PROCESS_TAG, nullptr));
#else
        const pid_t processId = fork();
        if (processId == 0)
        {
            failVerification();
            _exit(EXIT_SUCCESS);
        }

        int status = 0;
        waitpid(processId, &status, 0);
        return WEXITSTATUS(status);
#endif
    }
};

class CodedError final : public std::runtime_error
{
public:
    CodedError(const std::string& text, int code)
        : std::runtime_error(text)
        , m_code(code)
    {
    }

    int code() const noexcept
    {
        return m_code;
    }

private:
    int m_code;
};

class RecordedMessage final
{
public:
    explicit RecordedMessage(const LogMessage& message)
        : m_categoryName(message.category().name().c_str())
        , m_level(message.level())
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

    const std::string& text() const noexcept
    {
        return m_text;
    }

private:
    std::string m_categoryName;
    LogLevel m_level;
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

class DefaultLoggerFixture
{
public:
    DefaultLoggerFixture()
        : m_messages()
        , m_logger()
    {
        m_logger.addOutput(std::make_unique<RecordingOutput>(m_messages));
        m_logger.makeDefault();
    }

protected:
    std::vector<RecordedMessage> m_messages;
    Logger m_logger;
};

TEST_CASE_METHOD(DefaultLoggerFixture, "A failed verify reports the condition location and message", "[Assertion]")
{
    const int value = 2;

    const int expectedLine = __LINE__ + 1;
    HELLFIRE_VERIFY_LOG(value == 1, "value is {}", value);

    REQUIRE(m_messages.size() == 1);
    CHECK_THAT(m_messages[0].text(), StartsWith("verify condition 'value == 1' failed at: "));
    CHECK_THAT(m_messages[0].text(), ContainsSubstring(fmt::format("Assertion.cpp:{}: value is 2", expectedLine)));
}

TEST_CASE_METHOD(DefaultLoggerFixture, "A failed verify logs an ERROR in the Assertion category", "[Assertion]")
{
    HELLFIRE_VERIFY_LOG(false, "message");

    REQUIRE(m_messages.size() == 1);
    CHECK(m_messages[0].level() == LogLevel::ERROR);
    CHECK(m_messages[0].categoryName() == "Assertion");
}

TEST_CASE_METHOD(DefaultLoggerFixture, "A failed verify accepts a message without arguments", "[Assertion]")
{
    HELLFIRE_VERIFY_LOG(false, "plain message");

    REQUIRE(m_messages.size() == 1);
    CHECK_THAT(m_messages[0].text(), ContainsSubstring(": plain message"));
}

TEST_CASE_METHOD(DefaultLoggerFixture, "A failed verify accepts positional arguments", "[Assertion]")
{
    HELLFIRE_VERIFY_LOG(false, "{1} before {0}", "second", "first");

    REQUIRE(m_messages.size() == 1);
    CHECK_THAT(m_messages[0].text(), ContainsSubstring(": first before second"));
}

TEST_CASE_METHOD(DefaultLoggerFixture, "A failed verify accepts lvalue and rvalue arguments", "[Assertion]")
{
    int mutableValue = 1;
    const std::string constantValue = "two";

    HELLFIRE_VERIFY_LOG(false, "{} {} {}", mutableValue, constantValue, 3.5);

    REQUIRE(m_messages.size() == 1);
    CHECK_THAT(m_messages[0].text(), ContainsSubstring(": 1 two 3.5"));
}

TEST_CASE_METHOD(DefaultLoggerFixture, "A failed verify keeps braces in the condition text", "[Assertion]")
{
    HELLFIRE_VERIFY_LOG(std::string{"{}"}.empty(), "message");

    REQUIRE(m_messages.size() == 1);
    CHECK_THAT(m_messages[0].text(), StartsWith(R"(verify condition 'std::string{"{}"}.empty()' failed at: )"));
}

TEST_CASE_METHOD(DefaultLoggerFixture, "A failed verify reports a macro in the condition without expanding it", "[Assertion]")
{
    HELLFIRE_VERIFY_LOG(HELLFIRE_TESTS_FALSE_CONDITION, "message");

    REQUIRE(m_messages.size() == 1);
    CHECK_THAT(m_messages[0].text(), StartsWith("verify condition 'HELLFIRE_TESTS_FALSE_CONDITION' failed at: "));
}

TEST_CASE_METHOD(DefaultLoggerFixture, "A failed verify includes a stack trace only in debug builds", "[Assertion]")
{
    HELLFIRE_VERIFY_LOG(false, "message");

    REQUIRE(m_messages.size() == 1);
#if defined(__cpp_lib_stacktrace) && defined(HELLFIRE_DEBUG)
    CHECK_THAT(m_messages[0].text(), ContainsSubstring(": message\nstacktrace:\n"));
#else
    CHECK_THAT(m_messages[0].text(), EndsWith(": message"));
#endif
}

TEST_CASE_METHOD(DefaultLoggerFixture, "A verify evaluates its condition exactly once", "[Assertion]")
{
    const bool condition = GENERATE(true, false);
    int evaluationCount = 0;
    const auto evaluate = [&evaluationCount, condition] {
        ++evaluationCount;
        return condition;
    };

    HELLFIRE_VERIFY_LOG(evaluate(), "message");

    CHECK(evaluationCount == 1);
}

TEST_CASE_METHOD(DefaultLoggerFixture, "A verify evaluates its format arguments only on failure", "[Assertion]")
{
    const auto [condition, expectedEvaluationCount] = GENERATE(table<bool, int>({
        {true, 0},
        {false, 1},
    }));
    int evaluationCount = 0;

    HELLFIRE_VERIFY_LOG(condition, "value {}", ++evaluationCount);

    CHECK(evaluationCount == expectedEvaluationCount);
}

TEST_CASE_METHOD(DefaultLoggerFixture, "An exception from a format argument reaches the caller", "[Assertion]")
{
    const auto failingArgument = []() -> int { throw std::runtime_error{"argument failed"}; };
    const auto verify = [&failingArgument] { HELLFIRE_VERIFY_LOG(false, "value {}", failingArgument()); };

    CHECK_THROWS_AS(verify(), std::runtime_error);
    CHECK(m_messages.empty());
}

TEST_CASE_METHOD(DefaultLoggerFixture, "A failed verify keeps its condition and location when the message cannot be formatted", "[Assertion]")
{
    const int expectedLine = __LINE__ + 1;
    HELLFIRE_VERIFY_LOG(false, "value {}", UnformattableValue{});

    REQUIRE(m_messages.size() == 1);
    CHECK_THAT(m_messages[0].text(), StartsWith("verify condition 'false' failed at: "));
    CHECK_THAT(m_messages[0].text(), ContainsSubstring(fmt::format("Assertion.cpp:{}: <message could not be formatted>", expectedLine)));
}

TEST_CASE("A failed verify writes to stderr when there is no default logger", "[Assertion]")
{
    REQUIRE(Logger::defaultLogger() == nullptr);
    const CapturedStream standardError{stderr};

    HELLFIRE_VERIFY_LOG(false, "message {}", 1);

    const auto text = standardError.text();
    CHECK_THAT(text, StartsWith("verify condition 'false' failed at: "));
    CHECK_THAT(text, ContainsSubstring(": message 1"));
    CHECK_THAT(text, EndsWith("\n"));
}

TEST_CASE("A failed verify survives a stderr that rejects writes", "[Assertion]")
{
    REQUIRE(Logger::defaultLogger() == nullptr);
    // Larger than the stream buffer, so the write fails right away instead of on a later flush.
    const std::string longMessage(1'000'000, 'x');
    std::string writtenText;

    {
        const CapturedStream standardError{stderr, fmt::file::RDONLY | fmt::file::CREATE};
        HELLFIRE_VERIFY_LOG(false, "{}", longMessage);
        writtenText = standardError.text();
    }

    CHECK(writtenText.empty());
}

TEST_CASE("A failed verify ignores a logger that is not the default", "[Assertion]")
{
    std::vector<RecordedMessage> messages;
    Logger logger;
    logger.addOutput(std::make_unique<RecordingOutput>(messages));
    const CapturedStream standardError{stderr};

    HELLFIRE_VERIFY_LOG(false, "message");

    CHECK(messages.empty());
    CHECK_THAT(standardError.text(), ContainsSubstring(": message"));
}

TEST_CASE("A failed verify writes to stderr after the default logger is destroyed", "[Assertion]")
{
    std::vector<RecordedMessage> messages;
    {
        Logger logger;
        logger.addOutput(std::make_unique<RecordingOutput>(messages));
        logger.makeDefault();
    }
    const CapturedStream standardError{stderr};

    HELLFIRE_VERIFY_LOG(false, "message");

    CHECK(messages.empty());
    CHECK_THAT(standardError.text(), ContainsSubstring(": message"));
}

TEST_CASE_METHOD(DefaultLoggerFixture, "A failed verify reaches the default logger after another logger is destroyed", "[Assertion]")
{
    {
        const Logger otherLogger;
    }

    HELLFIRE_VERIFY_LOG(false, "message");

    CHECK(m_messages.size() == 1);
}

TEST_CASE_METHOD(DefaultLoggerFixture, "A failed verify reaches the most recent default logger", "[Assertion]")
{
    std::vector<RecordedMessage> secondMessages;
    {
        Logger secondLogger;
        secondLogger.addOutput(std::make_unique<RecordingOutput>(secondMessages));
        secondLogger.makeDefault();

        HELLFIRE_VERIFY_LOG(false, "message");
    }

    CHECK(m_messages.empty());
    CHECK(secondMessages.size() == 1);
    CHECK(Logger::defaultLogger() == nullptr);
}

TEST_CASE_METHOD(DefaultLoggerFixture, "Failed verifies from concurrent threads all reach the default logger", "[Assertion]")
{
    constexpr std::size_t THREAD_COUNT = 8;
    constexpr std::size_t FAILURES_PER_THREAD = 25;

    std::vector<std::thread> threads;
    for (std::size_t threadIndex = 0; threadIndex < THREAD_COUNT; ++threadIndex)
    {
        threads.emplace_back([] {
            for (std::size_t failureIndex = 0; failureIndex < FAILURES_PER_THREAD; ++failureIndex)
            {
                HELLFIRE_VERIFY_LOG(false, "failure {}", failureIndex);
            }
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    CHECK(m_messages.size() == THREAD_COUNT * FAILURES_PER_THREAD);
}

TEST_CASE_METHOD(DefaultLoggerFixture, "HELLFIRE_VERIFY_LOG reports a failure and carries on", "[Assertion]")
{
    const auto [condition, expectedMessageCount] = GENERATE(table<bool, std::size_t>({
        {true, 0u},
        {false, 1u},
    }));
    bool carriedOn = false;

    HELLFIRE_VERIFY_LOG(condition, "message");
    carriedOn = true;

    CHECK(carriedOn);
    CHECK(m_messages.size() == expectedMessageCount);
}

TEST_CASE_METHOD(DefaultLoggerFixture, "HELLFIRE_VERIFY_RETURN_VOID returns only on failure", "[Assertion]")
{
    const auto [condition, expectedMessageCount] = GENERATE(table<bool, std::size_t>({
        {true, 0u},
        {false, 1u},
    }));
    bool reachedEnd = false;
    const auto verify = [&reachedEnd](bool verifiedCondition) {
        HELLFIRE_VERIFY_RETURN_VOID(verifiedCondition, "message");
        reachedEnd = true;
    };

    verify(condition);

    CHECK(reachedEnd == condition);
    CHECK(m_messages.size() == expectedMessageCount);
}

TEST_CASE_METHOD(DefaultLoggerFixture, "HELLFIRE_VERIFY_RETURN_VALUE returns the value only on failure", "[Assertion]")
{
    const auto [condition, expectedResult, expectedMessageCount] = GENERATE(table<bool, int, std::size_t>({
        {true, 1, 0u},
        {false, -1, 1u},
    }));
    const auto verify = [](bool verifiedCondition) {
        HELLFIRE_VERIFY_RETURN_VALUE(verifiedCondition, -1, "value {}", 5);
        return 1;
    };

    CHECK(verify(condition) == expectedResult);
    CHECK(m_messages.size() == expectedMessageCount);
}

TEST_CASE_METHOD(DefaultLoggerFixture, "HELLFIRE_VERIFY_RETURN_VALUE accepts a braced value", "[Assertion]")
{
    const auto verify = []() -> std::string {
        HELLFIRE_VERIFY_RETURN_VALUE(false, {}, "message");
        return "verified";
    };

    CHECK(verify().empty());
}

TEST_CASE("HELLFIRE_VERIFY_RETURN_VALUE works in constant evaluation while the condition holds", "[Assertion]")
{
    constexpr auto verify = [](int value) {
        HELLFIRE_VERIFY_RETURN_VALUE(value > 0, -1, "value {}", value);
        return value;
    };

    STATIC_REQUIRE(verify(3) == 3);
}

TEST_CASE_METHOD(DefaultLoggerFixture, "HELLFIRE_VERIFY_CONTINUE skips the rest of the loop body on failure", "[Assertion]")
{
    std::vector<int> visitedIndices;

    for (int index = 0; index < 4; ++index)
    {
        HELLFIRE_VERIFY_CONTINUE(index % 2 == 0, "skipped {}", index);
        visitedIndices.push_back(index);
    }

    CHECK(visitedIndices == std::vector<int>{0, 2});
    REQUIRE(m_messages.size() == 2);
    CHECK_THAT(m_messages[0].text(), ContainsSubstring(": skipped 1"));
    CHECK_THAT(m_messages[1].text(), ContainsSubstring(": skipped 3"));
}

TEST_CASE("HELLFIRE_VERIFY_CONTINUE leaves a following else attached to the outer if", "[Assertion]")
{
    int elseBranchCount = 0;

    for (const bool outerCondition : {true, false})
    {
        if (outerCondition)
            HELLFIRE_VERIFY_CONTINUE(true, "message");
        else
            ++elseBranchCount;
    }

    CHECK(elseBranchCount == 1);
}

TEST_CASE_METHOD(DefaultLoggerFixture, "HELLFIRE_VERIFY_THROW reports the failure and throws only on failure", "[Assertion]")
{
    const auto verify = [](bool condition) { HELLFIRE_VERIFY_THROW(condition, CodedError, "text", 7); };

    CHECK_NOTHROW(verify(true));
    CHECK(m_messages.empty());

    CHECK_THROWS_AS(verify(false), CodedError);

    REQUIRE(m_messages.size() == 1);
    CHECK_THAT(m_messages[0].text(), StartsWith("verify condition 'condition' failed at: "));
    CHECK_THAT(m_messages[0].text(), ContainsSubstring(": throwing CodedError"));
}

TEST_CASE_METHOD(DefaultLoggerFixture, "HELLFIRE_VERIFY_THROW passes its arguments to the exception", "[Assertion]")
{
    int code = 0;
    std::string text;

    try
    {
        HELLFIRE_VERIFY_THROW(false, CodedError, "text", 7);
    }
    catch (const CodedError& error)
    {
        code = error.code();
        text = error.what();
    }

    CHECK(code == 7);
    CHECK(text == "text");
}

TEST_CASE_METHOD(DefaultLoggerFixture, "HELLFIRE_VERIFY_THROW accepts an exception without arguments", "[Assertion]")
{
    const auto verify = [] { HELLFIRE_VERIFY_THROW(false, std::bad_alloc); };

    CHECK_THROWS_AS(verify(), std::bad_alloc);
}

#ifdef __cpp_lib_expected
TEST_CASE_METHOD(DefaultLoggerFixture, "HELLFIRE_VERIFY_RETURN_UNEXPECTED returns the error without reporting it", "[Assertion]")
{
    // A condition known at compile time lets GCC evaluate the passing call during compilation, so it would never run.
    bool condition = true;
    const auto verify = [](bool verifiedCondition) -> std::expected<int, std::string> {
        HELLFIRE_VERIFY_RETURN_UNEXPECTED(verifiedCondition, "failure");
        return 1;
    };

    const auto verified = verify(condition);
    condition = false;
    const auto failed = verify(condition);

    REQUIRE(verified.has_value());
    CHECK(*verified == 1);
    REQUIRE_FALSE(failed.has_value());
    CHECK(failed.error() == "failure");
    CHECK(m_messages.empty());
}
#endif

TEST_CASE("HELLFIRE_VERIFY_TERMINATE does nothing while the condition holds", "[Assertion]")
{
    TerminatingProcess::verify(true);

    SUCCEED();
}

TEST_CASE("HELLFIRE_VERIFY_TERMINATE reports the failure and terminates the process", "[Assertion]")
{
    int exitCode = 0;
    std::string text;

    {
        const CapturedStream standardOutput{stdout};
        const CapturedStream standardError{stderr};
        exitCode = TerminatingProcess::run();
        text = standardError.text();
    }

    CHECK(exitCode == TERMINATE_EXIT_CODE);
    CHECK_THAT(text, StartsWith("verify condition 'condition' failed at: "));
    CHECK_THAT(text, ContainsSubstring(": fatal 7"));
}

TEST_CASE("HELLFIRE_VERIFY_TERMINATE child process", TERMINATE_PROCESS_TAG)
{
    TerminatingProcess::failVerification();
}
} // namespace Hellfire::Engine::AssertionTests
