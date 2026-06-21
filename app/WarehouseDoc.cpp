#include "framework.h"
#include "resource.h"

#include <exception>
#include <memory>

#include "TextUtil.h"
#include "WarehouseDoc.h"
#include "warehouse/connection_profiles.hpp"
#include "warehouse/movement_command.hpp"

IMPLEMENT_DYNCREATE(CWarehouseDoc, CDocument)

BEGIN_MESSAGE_MAP(CWarehouseDoc, CDocument)
END_MESSAGE_MAP()

CWarehouseDoc::CWarehouseDoc() {}

warehouse::StockRepository& CWarehouseDoc::repository() {
    if (!repo_) {
        repo_ = std::make_unique<warehouse::StockRepository>(warehouse::demoConnectionString());
    }
    return *repo_;
}

BOOL CWarehouseDoc::OnNewDocument() {
    if (!CDocument::OnNewDocument()) {
        return FALSE;
    }
    Refresh();
    return TRUE;
}

void CWarehouseDoc::Refresh() {
    try {
        stock_ = repository().loadCurrentStock();
        UpdateAllViews(nullptr);
    } catch (const std::exception& error) {
        AfxMessageBox(FromUtf8(error.what()), MB_ICONERROR);
    }
}

int CWarehouseDoc::onHandOf(int productId, int warehouseId) const {
    for (const warehouse::StockRow& row : stock_) {
        if (row.productId == productId && row.warehouseId == warehouseId) {
            return row.onHand;
        }
    }
    return 0;
}

void CWarehouseDoc::ExecuteMovement(const warehouse::Movement& movement) {
    try {
        commands_.execute(std::make_unique<warehouse::MovementCommand>(movement, repository()));
        Refresh();
        CString message;
        message.Format(_T("Zarejestrowano. Nowy stan: %d"),
                       onHandOf(movement.productId, movement.warehouseId));
        speech_.Say(message);
    } catch (const std::exception& error) {
        AfxMessageBox(FromUtf8(error.what()), MB_ICONERROR);
    }
}

void CWarehouseDoc::Undo() {
    try {
        commands_.undo();
        Refresh();
        speech_.Say(_T("Cofnięto"));
    } catch (const std::exception& error) {
        AfxMessageBox(FromUtf8(error.what()), MB_ICONERROR);
    }
}

void CWarehouseDoc::Redo() {
    try {
        commands_.redo();
        Refresh();
        speech_.Say(_T("Ponowiono"));
    } catch (const std::exception& error) {
        AfxMessageBox(FromUtf8(error.what()), MB_ICONERROR);
    }
}
