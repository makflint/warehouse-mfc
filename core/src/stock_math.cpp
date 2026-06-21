#include "warehouse/stock_math.hpp"

#include <numeric>

namespace warehouse {

int onHand(const std::vector<int>& signedQuantities) {
    return std::accumulate(signedQuantities.begin(), signedQuantities.end(), 0);
}

bool isLow(int onHand, int reorderLevel) {
    return onHand <= reorderLevel;
}

}  // namespace warehouse
