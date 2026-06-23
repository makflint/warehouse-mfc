#pragma once

#include <vector>

#include "warehouse/stock_row.hpp"

namespace warehouse {

// Pure decisions the MFC views make, extracted from app/ so they can be unit-tested
// without a GUI: how each grid column sorts, and which combo item to pre-select.

// Columns of the main stock grid, in display order (see CStockView).
enum class StockColumn { Warehouse, Sku, Product, OnHand };

// Columns of the movement-log list, in display order (see CMovementLogList).
enum class MovementColumn { Time, Warehouse, Type, Sku, Qty };

// Three-way comparison (<0 / 0 / >0) of two stock rows by the given column. Text
// columns compare lexicographically; Stan (OnHand) compares numerically so 9 sorts
// before 100. Unknown columns compare equal. Sort direction is the caller's concern.
int compareStock(const StockRow& a, const StockRow& b, StockColumn column);

// Three-way comparison of two movement rows. Ilość (Qty) compares by |qty| — the
// magnitude the grid shows; createdAt is ISO text so its lexical order is chronological.
int compareMovement(const MovementRow& a, const MovementRow& b, MovementColumn column);

// The combo index to pre-select when its options carry these ids: the position of
// the first id equal to targetId, or 0 (the first item) when none matches.
int selectedIndexForId(const std::vector<int>& ids, int targetId);

}  // namespace warehouse
