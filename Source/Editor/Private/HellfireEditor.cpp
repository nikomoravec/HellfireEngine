#include "Logger.hpp"

#include <memory>

constexpr Hellfire::Engine::LogCategory HF_EDITOR{"Editor"};

int main()
{
    Hellfire::Engine::Logger l{};
    l.addOutput(std::make_unique<Hellfire::Engine::LogConsoleOutput>());
    l.addOutput(std::make_unique<Hellfire::Engine::LogFileOutput>(fmt::output_file("OutputLog.txt")));

    l.info(HF_EDITOR, "Hello, {}", "Hellfire!");
    return 0;
}
