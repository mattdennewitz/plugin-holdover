#include <catch2/catch_test_macros.hpp>

TEST_CASE("build harness is alive", "[smoke]") {
    REQUIRE(1 + 1 == 2);
}
