add_executable(HellfireEngineTests
    Assertion.cpp
    Hash.cpp
    Logger.cpp
    Rotator.cpp
    StringId.cpp
    Transform.cpp
    Units.cpp
)

target_link_libraries(HellfireEngineTests PRIVATE
    Hellfire::Engine
    Catch2::Catch2WithMain
)

catch_discover_tests(HellfireEngineTests)
