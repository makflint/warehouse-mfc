#include "catch_amalgamated.hpp"
#include "warehouse/view_logic.hpp"

using warehouse::compareMovement;
using warehouse::compareStock;
using warehouse::MovementColumn;
using warehouse::MovementRow;
using warehouse::selectedIndexForId;
using warehouse::StockColumn;
using warehouse::StockRow;

namespace {
StockRow stockRow(const std::string& wh, const std::string& sku, const std::string& name,
                  int onHand) {
    StockRow r;
    r.warehouseCode = wh;
    r.sku = sku;
    r.productName = name;
    r.onHand = onHand;
    return r;
}

MovementRow moveRow(const std::string& at, const std::string& wh, const std::string& type,
                    const std::string& sku, int qty) {
    MovementRow r;
    r.createdAt = at;
    r.warehouseCode = wh;
    r.type = type;
    r.sku = sku;
    r.qty = qty;
    return r;
}
}  // namespace

TEST_CASE("compareStock orders text columns lexicographically") {
    const StockRow a = stockRow("MAG-A", "S-1", "Alfa", 5);
    const StockRow b = stockRow("MAG-B", "S-2", "Beta", 5);
    REQUIRE(compareStock(a, b, StockColumn::Warehouse) < 0);
    REQUIRE(compareStock(b, a, StockColumn::Warehouse) > 0);
    REQUIRE(compareStock(a, b, StockColumn::Sku) < 0);
    REQUIRE(compareStock(a, b, StockColumn::Product) < 0);
    REQUIRE(compareStock(a, a, StockColumn::Warehouse) == 0);
}

TEST_CASE("compareStock orders Stan numerically, not as text") {
    const StockRow nine = stockRow("MAG-A", "S", "P", 9);
    const StockRow hundred = stockRow("MAG-A", "S", "P", 100);
    // Lexicographically "100" < "9"; numerically 9 < 100. We want numeric order.
    REQUIRE(compareStock(nine, hundred, StockColumn::OnHand) < 0);
    REQUIRE(compareStock(hundred, nine, StockColumn::OnHand) > 0);
}

TEST_CASE("compareMovement orders Ilosc by absolute value") {
    const MovementRow ship50 = moveRow("2026-01-01", "MAG-A", "OUT", "S", -50);
    const MovementRow recv10 = moveRow("2026-01-01", "MAG-A", "IN", "S", 10);
    REQUIRE(compareMovement(recv10, ship50, MovementColumn::Qty) < 0);   // |10| < |50|
    REQUIRE(compareMovement(ship50, recv10, MovementColumn::Qty) > 0);
    const MovementRow recv50 = moveRow("2026-01-01", "MAG-A", "IN", "S", 50);
    REQUIRE(compareMovement(ship50, recv50, MovementColumn::Qty) == 0);  // |-50| == |50|
}

TEST_CASE("compareMovement orders Czas chronologically via ISO text") {
    const MovementRow older = moveRow("2026-01-01 08:00:00", "MAG-A", "IN", "S", 1);
    const MovementRow newer = moveRow("2026-01-02 08:00:00", "MAG-A", "IN", "S", 1);
    REQUIRE(compareMovement(older, newer, MovementColumn::Time) < 0);
}

TEST_CASE("compareMovement orders the Magazyn / Ruch / Symbol text columns") {
    const MovementRow a = moveRow("2026-01-01", "MAG-A", "IN", "S-1", 1);
    const MovementRow b = moveRow("2026-01-01", "MAG-B", "OUT", "S-2", 1);
    REQUIRE(compareMovement(a, b, MovementColumn::Warehouse) < 0);
    REQUIRE(compareMovement(a, b, MovementColumn::Type) < 0);  // "IN" < "OUT"
    REQUIRE(compareMovement(a, b, MovementColumn::Sku) < 0);
}

TEST_CASE("compare* return 0 (stable) for an unknown column") {
    const StockRow s = stockRow("MAG-A", "S", "P", 5);
    const MovementRow m = moveRow("2026-01-01", "MAG-A", "IN", "S", 5);
    REQUIRE(compareStock(s, s, static_cast<StockColumn>(99)) == 0);
    REQUIRE(compareMovement(m, m, static_cast<MovementColumn>(99)) == 0);
}

TEST_CASE("selectedIndexForId finds the matching id, else falls back to 0") {
    const std::vector<int> ids = {4521, 4524, 4530};
    REQUIRE(selectedIndexForId(ids, 4521) == 0);
    REQUIRE(selectedIndexForId(ids, 4524) == 1);
    REQUIRE(selectedIndexForId(ids, 4530) == 2);
    REQUIRE(selectedIndexForId(ids, 9999) == 0);  // no match -> first item
    REQUIRE(selectedIndexForId({}, 4521) == 0);   // empty -> 0
}
