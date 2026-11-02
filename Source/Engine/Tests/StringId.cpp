#include "StringId.hpp"

#include "catch2/catch_test_macros.hpp"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

namespace Hellfire::Engine::StringIdTests
{
TEST_CASE("Default StringId is empty and equals the id of an empty string", "[StringId]")
{
    CHECK(StringId{}.isEmpty());
    CHECK(StringId{""}.isEmpty());
    CHECK(StringId{} == StringId{""});
    CHECK(StringId{}.value() == 0xCBF29CE484222325u);
}

TEST_CASE("StringId value is the FNV-1a hash of its text", "[StringId]")
{
    CHECK(StringId{"a"}.value() == 0xAF63DC4C8601EC8Cu);
    CHECK(StringId{"foobar"}.value() == 0x85944171F73967E8u);
}

TEST_CASE("StringId works in constant evaluation", "[StringId]")
{
    constexpr StringId id{"foobar"};

    STATIC_CHECK(id.value() == 0x85944171F73967E8u);
    STATIC_CHECK(!id.isEmpty());
    STATIC_CHECK(id == StringId{"foobar"});
    STATIC_CHECK(StringId{} == StringId{""});
}

TEST_CASE("StringId of any non-empty text is not empty", "[StringId]")
{
    CHECK_FALSE(StringId{"a"}.isEmpty());
    CHECK_FALSE(StringId{" "}.isEmpty());
    CHECK_FALSE(StringId{std::string_view{"\0", 1}}.isEmpty());
}

TEST_CASE("StringId gives the same id for the same text however it was built", "[StringId]")
{
    const std::string text = std::string{"foo"} + "bar";
    constexpr std::string_view longer{"foobarbaz"};

    CHECK(StringId{text} == StringId{"foobar"});
    CHECK(StringId{longer.substr(0, 6)} == StringId{"foobar"});
}

TEST_CASE("StringId gives different ids for different text", "[StringId]")
{
    CHECK(StringId{"foo"} != StringId{"bar"});
    CHECK(StringId{"Player"} != StringId{"player"});
    CHECK(StringId{"a"} != StringId{});
}

TEST_CASE("StringId keeps null characters only when they are inside the view", "[StringId]")
{
    CHECK(StringId{std::string_view{"a\0b", 3}} != StringId{"a"});
    CHECK(StringId{"a\0b"} == StringId{"a"});
}

TEST_CASE("StringId orders by hash value, not alphabetically", "[StringId]")
{
    const StringId first{"a"};
    const StringId second{"foobar"};

    CHECK(second < first);
    CHECK(std::is_gt(first <=> second));
    CHECK(std::is_eq(first <=> StringId{"a"}));
}

TEST_CASE("std::hash of StringId is its value", "[StringId]")
{
    const StringId id{"foobar"};

    CHECK(std::hash<StringId>{}(id) == static_cast<std::size_t>(id.value()));
}

TEST_CASE("StringId works as a key in hashed and ordered containers", "[StringId]")
{
    const std::string text = std::string{"foo"} + "bar";
    const std::unordered_map<StringId, int> hashed{{StringId{"foobar"}, 1}};
    const std::map<StringId, int> ordered{{StringId{"foobar"}, 1}};
    const std::set<StringId> sorted{StringId{"a"}, StringId{"foobar"}};

    CHECK(hashed.contains(StringId{text}));
    CHECK(ordered.contains(StringId{text}));
    CHECK_FALSE(hashed.contains(StringId{"a"}));
    CHECK(*sorted.begin() == StringId{"foobar"});
}

TEST_CASE("StringId cannot be created implicitly from text", "[StringId]")
{
    STATIC_CHECK(!std::is_convertible_v<std::string_view, StringId>);
    STATIC_CHECK(!std::is_convertible_v<const char*, StringId>);
    STATIC_CHECK(!std::is_convertible_v<std::string, StringId>);
}

TEST_CASE("StringId is a trivially copyable 64-bit value", "[StringId]")
{
    STATIC_CHECK(std::is_trivially_copyable_v<StringId>);
    STATIC_CHECK(std::is_same_v<StringId::ValueType, std::uint64_t>);
    STATIC_CHECK(sizeof(StringId) == sizeof(std::uint64_t));
}
} // namespace Hellfire::Engine::StringIdTests
