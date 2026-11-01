cmake_minimum_required(VERSION 3.21)

project(Hellfire LANGUAGES C CXX)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

option(HELLFIRE_BUILD_TESTS "Build Hellfire tests" ON)

include(FetchContent)

FetchContent_Declare(cstring_view GIT_REPOSITORY https://github.com/bemanproject/cstring_view.git GIT_TAG v0.1.0)
FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt.git GIT_TAG 12.1.0)
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm GIT_TAG 1.0.3)

FetchContent_MakeAvailable(cstring_view)
FetchContent_MakeAvailable(fmt)
FetchContent_MakeAvailable(glm)

target_compile_definitions(glm INTERFACE
    GLM_ENABLE_EXPERIMENTAL
    GLM_FORCE_EXPLICIT_CTOR
)

if (HELLFIRE_BUILD_TESTS)
    enable_testing()

    FetchContent_Declare(Catch2 GIT_REPOSITORY https://github.com/catchorg/Catch2.git GIT_TAG v3.15.3)
    FetchContent_MakeAvailable(Catch2)

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)
endif()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Source/Engine")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Source/Editor")
