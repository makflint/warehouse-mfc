#ifndef WAREHOUSE_MOVEMENT_HPP
#define WAREHOUSE_MOVEMENT_HPP

namespace warehouse {

enum class MovementType { In, Out };

// A single stock movement. Qty is a positive count; the direction lives in Type
// (matching the DB stored proc, which derives the signed quantity from the type).
struct Movement {
    int productId;
    int warehouseId;
    int qty;
    MovementType type;
};

// The movement that exactly cancels m: same product/warehouse/quantity, flipped
// direction. Undoing an IN ships the goods back out, and vice versa.
Movement compensating(const Movement& m);

}  // namespace warehouse

#endif  // WAREHOUSE_MOVEMENT_HPP
