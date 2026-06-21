// Tiny console driver for the data/ layer (PLAN M2 manual check): connect to the
// seeded LocalDB, read current stock, record a movement, and show that an
// over-ship is rejected by sp_RecordMovement.
#include <windows.h>

#include <iostream>

#include "warehouse/connection_profiles.hpp"
#include "warehouse/stock_repository.hpp"

using warehouse::demoConnectionString;
using warehouse::Movement;
using warehouse::MovementType;
using warehouse::StockRepository;
using warehouse::StockRow;

int main() {
    ::SetConsoleOutputCP(CP_UTF8);
    try {
        StockRepository repo(demoConnectionString());

        const std::vector<StockRow> stock = repo.loadCurrentStock();
        std::cout << "Current stock (vCurrentStock):\n";
        for (const auto& row : stock) {
            std::cout << "  " << row.warehouseCode << "  " << row.sku << "  " << row.productName
                      << "  on-hand=" << row.onHand << (row.isLow ? "  [LOW]" : "") << "\n";
        }

        // Pick a real product/warehouse from the data (ids are assigned by the DB).
        const StockRow* target = nullptr;
        for (const auto& row : stock) {
            if (row.sku == "4521" && row.warehouseCode == "MAG-A") {
                target = &row;
            }
        }
        if (target == nullptr) {
            std::cerr << "FATAL: sku 4521 @ MAG-A not found in seed.\n";
            return 1;
        }
        const Movement in{target->productId, target->warehouseId, 5, MovementType::In};

        std::cout << "\nRecord IN 5 of 4521 @ MAG-A -> new on-hand = "
                  << repo.recordMovement(in) << "\n";
        std::cout << "Record OUT 5 (restore)       -> new on-hand = "
                  << repo.recordMovement(warehouse::compensating(in)) << "\n";

        std::cout << "\nTry OUT 9999 (must be rejected): ";
        try {
            repo.recordMovement(Movement{target->productId, target->warehouseId, 9999, MovementType::Out});
            std::cout << "ERROR - was NOT rejected!\n";
            return 2;
        } catch (const std::exception& expected) {
            std::cout << "rejected as expected ->" << expected.what() << "\n";
        }
        return 0;
    } catch (const std::exception& fatal) {
        std::cerr << "FATAL: " << fatal.what() << "\n";
        return 1;
    }
}
