#include "framework.h"
#include "resource.h"

#include <exception>

#include "TextUtil.h"
#include "WarehouseDoc.h"
#include "warehouse/connection_profiles.hpp"

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
