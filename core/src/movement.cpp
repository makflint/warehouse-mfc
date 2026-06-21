#include "warehouse/movement.hpp"

namespace warehouse {

Movement compensating(const Movement& m) {
    const MovementType flipped = (m.type == MovementType::In) ? MovementType::Out : MovementType::In;
    return Movement{m.productId, m.warehouseId, m.qty, flipped};
}

}  // namespace warehouse
