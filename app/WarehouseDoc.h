#pragma once

#include <memory>
#include <vector>

#include "framework.h"
#include "warehouse/command.hpp"
#include "warehouse/movement.hpp"
#include "warehouse/stock_repository.hpp"

// The document owns the data-layer connection, the current snapshot of stock,
// and the undo/redo history.
class CWarehouseDoc : public CDocument {
    DECLARE_DYNCREATE(CWarehouseDoc)

public:
    CWarehouseDoc();

    const std::vector<warehouse::StockRow>& Stock() const { return stock_; }

    // Reload vCurrentStock from the database and refresh the views.
    void Refresh();

    // Record a movement through the undo/redo stack, then refresh.
    void ExecuteMovement(const warehouse::Movement& movement);
    void Undo();
    void Redo();
    bool CanUndo() const { return commands_.canUndo(); }
    bool CanRedo() const { return commands_.canRedo(); }

protected:
    BOOL OnNewDocument() override;
    DECLARE_MESSAGE_MAP()

private:
    warehouse::StockRepository& repository();

    std::unique_ptr<warehouse::StockRepository> repo_;
    std::vector<warehouse::StockRow> stock_;
    warehouse::CommandStack commands_;
};
