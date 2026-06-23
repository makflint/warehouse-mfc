#pragma once

#include <string>

namespace warehouse {

// One row of vCurrentStock: a product x warehouse on-hand with the low-stock flag.
// Lives in core/ (pure data) so domain logic — sort comparisons, etc. — can be
// unit-tested without the data/ ODBC layer; data/ fills these from the database.
struct StockRow {
    int productId = 0;
    std::string sku;
    std::string productName;
    std::string unit;
    int reorderLevel = 0;
    int warehouseId = 0;
    std::string warehouseCode;
    std::string warehouseName;
    int onHand = 0;
    bool isLow = false;
};

// One entry of the append-only StockMovements log (most recent first).
struct MovementRow {
    std::string createdAt;      // "YYYY-MM-DD HH:MM:SS" (UTC)
    std::string type;           // "IN" / "OUT"
    std::string sku;
    std::string warehouseCode;
    int qty = 0;                // signed: + receive / - ship
};

}  // namespace warehouse
