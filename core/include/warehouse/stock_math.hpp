#ifndef WAREHOUSE_STOCK_MATH_HPP
#define WAREHOUSE_STOCK_MATH_HPP

#include <vector>

namespace warehouse {

// Current on-hand is derived from an append-only movement log: the sum of the
// signed movement quantities (+IN / -OUT). No mutable "quantity" column exists.
int onHand(const std::vector<int>& signedQuantities);

}  // namespace warehouse

#endif  // WAREHOUSE_STOCK_MATH_HPP
