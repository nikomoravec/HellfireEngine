file(GLOB_RECURSE HELLFIRE_ENGINE_SOURCES CONFIGURE_DEPENDS
    Public/*.hpp
    Public/*.cpp
    Private/*.hpp
    Private/*.cpp
)

add_library(Hellfire STATIC ${HELLFIRE_ENGINE_SOURCES})
add_library(Hellfire::Engine ALIAS Hellfire)

target_include_directories(Hellfire PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Public
)

target_include_directories(Hellfire PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

target_link_libraries(Hellfire PUBLIC
    fmt::fmt
    beman::cstring_view
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 14)
      target_link_libraries(Hellfire PRIVATE $<$<CONFIG:Debug>:stdc++exp>)
endif()

target_compile_definitions(Hellfire PUBLIC
    $<$<CONFIG:Debug>:HELLFIRE_DEBUG>
)

if (HELLFIRE_BUILD_TESTS)
    add_subdirectory(Tests)
endif()
