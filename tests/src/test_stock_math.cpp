#include "catch_amalgamated.hpp"
#include "warehouse/stock_math.hpp"

TEST_CASE("onHand sums signed movement quantities") {
    REQUIRE(warehouse::onHand({}) == 0);
    REQUIRE(warehouse::onHand({40, -5}) == 35);
}

TEST_CASE("isLow flags stock at or below the reorder level") {
    REQUIRE(warehouse::isLow(30, 50));          // below
    REQUIRE_FALSE(warehouse::isLow(35, 10));    // above
    REQUIRE(warehouse::isLow(10, 10));          // boundary counts as low
}
