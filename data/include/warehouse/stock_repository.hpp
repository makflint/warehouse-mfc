#ifndef WAREHOUSE_STOCK_REPOSITORY_HPP
#define WAREHOUSE_STOCK_REPOSITORY_HPP

#include <memory>
#include <string>
#include <vector>

#include "warehouse/movement.hpp"
#include "warehouse/movement_command.hpp"  // IMovementRecorder
#include "warehouse/stock_row.hpp"         // StockRow, MovementRow (pure domain rows)

namespace warehouse {

// ODBC-backed access to the Warehouse database. Implements IMovementRecorder so
// MovementCommand/CommandStack record through the same path as direct calls.
// ODBC headers stay hidden behind a pImpl so this header is MFC/Windows-clean.
class StockRepository : public IMovementRecorder {
public:
    explicit StockRepository(const std::string& connectionString);
    ~StockRepository() override;

    StockRepository(const StockRepository&) = delete;
    StockRepository& operator=(const StockRepository&) = delete;

    std::vector<StockRow> loadCurrentStock();

    // The most recent movements (newest first), joined to sku + warehouse code.
    std::vector<MovementRow> loadRecentMovements(int limit = 50);

    // Calls sp_RecordMovement and returns the new on-hand. Throws std::runtime_error
    // if the proc rejects the movement (insufficient stock -> THROW 50001).
    int recordMovement(const Movement& movement);

    // IMovementRecorder: record through the stored proc (return value discarded).
    void record(const Movement& movement) override { recordMovement(movement); }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace warehouse

#endif  // WAREHOUSE_STOCK_REPOSITORY_HPP
