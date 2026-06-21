#ifndef WAREHOUSE_STOCK_MATH_HPP
#define WAREHOUSE_STOCK_MATH_HPP

#include <vector>

namespace warehouse {

// Current on-hand is derived from an append-only movement log: the sum of the
// signed movement quantities (+IN / -OUT). No mutable "quantity" column exists.
int onHand(const std::vector<int>& signedQuantities);

// A product is low on stock when its on-hand has fallen to or below the reorder
// level (the boundary value counts as low).
bool isLow(int onHand, int reorderLevel);

}  // namespace warehouse

#endif  // WAREHOUSE_STOCK_MATH_HPP
