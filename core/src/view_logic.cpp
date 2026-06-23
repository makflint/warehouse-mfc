#include "warehouse/view_logic.hpp"

namespace warehouse {
namespace {

int cmp(int a, int b) { return a < b ? -1 : (a > b ? 1 : 0); }

int absValue(int x) { return x < 0 ? -x : x; }

}  // namespace

int compareStock(const StockRow& a, const StockRow& b, StockColumn column) {
    switch (column) {
        case StockColumn::Warehouse: return a.warehouseCode.compare(b.warehouseCode);
        case StockColumn::Sku:       return a.sku.compare(b.sku);
        case StockColumn::Product:   return a.productName.compare(b.productName);
        case StockColumn::OnHand:    return cmp(a.onHand, b.onHand);
    }
    return 0;
}

int compareMovement(const MovementRow& a, const MovementRow& b, MovementColumn column) {
    switch (column) {
        case MovementColumn::Time:      return a.createdAt.compare(b.createdAt);
        case MovementColumn::Warehouse: return a.warehouseCode.compare(b.warehouseCode);
        case MovementColumn::Type:      return a.type.compare(b.type);
        case MovementColumn::Sku:       return a.sku.compare(b.sku);
        case MovementColumn::Qty:       return cmp(absValue(a.qty), absValue(b.qty));
    }
    return 0;
}

int selectedIndexForId(const std::vector<int>& ids, int targetId) {
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (ids[i] == targetId) {
            return static_cast<int>(i);
        }
    }
    return 0;  // no match (or empty) -> first item
}

}  // namespace warehouse
