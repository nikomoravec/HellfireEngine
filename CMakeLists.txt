cmake_minimum_required(VERSION 3.21)

project(Hellfire LANGUAGES C CXX)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include(FetchContent)

FetchContent_Declare(cstring_view GIT_REPOSITORY https://github.com/bemanproject/cstring_view.git GIT_TAG v0.1.0)
FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt.git GIT_TAG 12.1.0)

FetchContent_MakeAvailable(cstring_view)
FetchContent_MakeAvailable(fmt)
