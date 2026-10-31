file(GLOB_RECURSE HELLFIRE_EDITOR_SOURCES CONFIGURE_DEPENDS
    Public/*.hpp
    Public/*.cpp
    Private/*.hpp
    Private/*.cpp
)

add_executable(HellfireEditor ${HELLFIRE_EDITOR_SOURCES})
