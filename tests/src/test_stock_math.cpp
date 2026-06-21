#include "catch_amalgamated.hpp"
#include "warehouse/stock_math.hpp"

TEST_CASE("onHand sums signed movement quantities") {
    REQUIRE(warehouse::onHand({}) == 0);
    REQUIRE(warehouse::onHand({40, -5}) == 35);
}
