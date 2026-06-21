#pragma once

#include <memory>
#include <vector>

#include "framework.h"
#include "warehouse/stock_repository.hpp"

// The document owns the data-layer connection and the current snapshot of stock.
class CWarehouseDoc : public CDocument {
    DECLARE_DYNCREATE(CWarehouseDoc)

public:
    CWarehouseDoc();

    const std::vector<warehouse::StockRow>& Stock() const { return stock_; }

    // Reload vCurrentStock from the database and refresh the views.
    void Refresh();

protected:
    BOOL OnNewDocument() override;
    DECLARE_MESSAGE_MAP()

private:
    warehouse::StockRepository& repository();

    std::unique_ptr<warehouse::StockRepository> repo_;
    std::vector<warehouse::StockRow> stock_;
};
