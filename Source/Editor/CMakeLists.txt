file(GLOB_RECURSE HELLFIRE_EDITOR_SOURCES CONFIGURE_DEPENDS
    Public/*.hpp
    Public/*.cpp
    Private/*.hpp
    Private/*.cpp
)

add_executable(HellfireEditor ${HELLFIRE_EDITOR_SOURCES})

target_link_libraries(HellfireEditor
    Hellfire::Engine
)

if(MINGW)
    target_link_options(HellfireEditor PRIVATE -static)
endif()
