#include "catch_amalgamated.hpp"
#include "warehouse/movement.hpp"

using warehouse::Movement;
using warehouse::MovementType;

TEST_CASE("compensating flips direction, keeps product/warehouse/quantity") {
    const Movement in{1, 2, 5, MovementType::In};
    const Movement c = warehouse::compensating(in);

    REQUIRE(c.type == MovementType::Out);
    REQUIRE(c.qty == 5);
    REQUIRE(c.productId == 1);
    REQUIRE(c.warehouseId == 2);

    REQUIRE(warehouse::compensating(c).type == MovementType::In);  // round-trip
}
