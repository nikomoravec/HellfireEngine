#include "Hash.hpp"

#include "catch2/catch_test_macros.hpp"

#include <string>
#include <string_view>

namespace Hellfire::Engine::HashTests
{
TEST_CASE("Hash::fnv1a matches the FNV-1a reference vectors", "[Hash]")
{
    CHECK(Hash::fnv1a("") == 0xCBF29CE484222325u);
    CHECK(Hash::fnv1a("a") == 0xAF63DC4C8601EC8Cu);
    CHECK(Hash::fnv1a("foobar") == 0x85944171F73967E8u);
}

TEST_CASE("Hash::fnv1a works in constant evaluation", "[Hash]")
{
    STATIC_CHECK(Hash::fnv1a("") == 0xCBF29CE484222325u);
    STATIC_CHECK(Hash::fnv1a("a") == 0xAF63DC4C8601EC8Cu);
    STATIC_CHECK(Hash::fnv1a("foobar") == 0x85944171F73967E8u);
}

TEST_CASE("Hash::fnv1a treats bytes above 0x7F as unsigned", "[Hash]")
{
    CHECK(Hash::fnv1a("\xFF") == 0xAF64724C8602EB6Eu);
    CHECK(Hash::fnv1a("\xC3\xA9") == 0x0AC21707B7181E01u);
}

TEST_CASE("Hash::fnv1a gives the same result for text built at runtime", "[Hash]")
{
    const std::string text = std::string{"foo"} + "bar";

    CHECK(Hash::fnv1a(text) == 0x85944171F73967E8u);
}

TEST_CASE("Hash::fnv1a hashes exactly the characters inside the view", "[Hash]")
{
    constexpr std::string_view text{"foobar"};

    CHECK(Hash::fnv1a(text.substr(0, 3)) == 0xDCB27518FED9D577u);
    CHECK(Hash::fnv1a(std::string_view{"a\0b", 3}) == 0xE5D29919042666B2u);
    CHECK(Hash::fnv1a(std::string_view{"a\0", 2}) == 0x089BE207B544F1E4u);
    CHECK(Hash::fnv1a(std::string_view{"a\0", 2}) != Hash::fnv1a("a"));
}
} // namespace Hellfire::Engine::HashTests
