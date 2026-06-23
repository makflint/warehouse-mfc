#include "framework.h"
#include "resource.h"

#include <exception>
#include <memory>

#include "I18n.h"
#include "TextUtil.h"
#include "WarehouseDoc.h"
#include "warehouse/connection_profiles.hpp"
#include "warehouse/db_error.hpp"
#include "warehouse/movement_command.hpp"

IMPLEMENT_DYNCREATE(CWarehouseDoc, CDocument)

BEGIN_MESSAGE_MAP(CWarehouseDoc, CDocument)
END_MESSAGE_MAP()

CWarehouseDoc::CWarehouseDoc() {}

void CWarehouseDoc::ShowError(const std::exception& error) {
    // cleanDbError (core/, unit-tested) strips the ODBC [SQLSTATE][driver][server] prefixes
    // so only the SQL Server RAISERROR text reaches the user.
    const CString message = FromUtf8(warehouse::cleanDbError(error.what()));
    if (CWnd* main = AfxGetMainWnd()) {
        main->MessageBox(message, i18n::T(i18n::AppTitle), MB_ICONERROR | MB_OK);
    } else {
        ::MessageBox(nullptr, message, i18n::T(i18n::AppTitle), MB_ICONERROR | MB_OK);
    }
}

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
    SetTitle(i18n::T(i18n::AppTitle));  // window title, instead of the default "Untitled"
    Refresh();
    return TRUE;
}

void CWarehouseDoc::Refresh() {
    try {
        stock_ = repository().loadCurrentStock();
        movements_ = repository().loadRecentMovements();
        UpdateAllViews(nullptr);
    } catch (const std::exception& error) {
        ShowError(error);
    }
}

void CWarehouseDoc::ExecuteMovement(const warehouse::Movement& movement) {
    try {
        commands_.execute(std::make_unique<warehouse::MovementCommand>(movement, repository()));
        Refresh();
    } catch (const std::exception& error) {
        ShowError(error);
    }
}

void CWarehouseDoc::Undo() {
    try {
        commands_.undo();
        Refresh();
    } catch (const std::exception& error) {
        ShowError(error);
    }
}

void CWarehouseDoc::Redo() {
    try {
        commands_.redo();
        Refresh();
    } catch (const std::exception& error) {
        ShowError(error);
    }
}
