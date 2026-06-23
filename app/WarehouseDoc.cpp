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

namespace {
// ODBC diagnostics arrive as "context\n  [SQLSTATE] [driver][server]message". Keep only
// the message text of each diagnostic line (dropping the context line and the bracketed
// prefixes) so the user sees the SQL Server RAISERROR text, not driver plumbing.
CString CleanDbError(const CString& raw) {
    CString result;
    int start = 0;
    while (start <= raw.GetLength()) {
        const int nl = raw.Find(_T('\n'), start);
        CString line = (nl < 0) ? raw.Mid(start) : raw.Mid(start, nl - start);
        line.Trim();
        if (!line.IsEmpty() && line[0] == _T('[')) {  // a diagnostic line; strip [..] prefixes
            int pos = 0;
            while (pos < line.GetLength() && line[pos] == _T('[')) {
                const int close = line.Find(_T(']'), pos);
                if (close < 0) {
                    break;
                }
                pos = close + 1;
                while (pos < line.GetLength() && line[pos] == _T(' ')) {
                    ++pos;
                }
            }
            CString text = line.Mid(pos);
            text.Trim();
            if (!text.IsEmpty()) {
                if (!result.IsEmpty()) {
                    result += _T("\n");
                }
                result += text;
            }
        }
        if (nl < 0) {
            break;
        }
        start = nl + 1;
    }
    return result.IsEmpty() ? raw : result;
}
}  // namespace

CWarehouseDoc::CWarehouseDoc() {}

void CWarehouseDoc::ShowError(const std::exception& error) {
    const CString message = CleanDbError(FromUtf8(error.what()));
    if (CWnd* main = AfxGetMainWnd()) {
        main->MessageBox(message, kAppTitle, MB_ICONERROR | MB_OK);
    } else {
        ::MessageBox(nullptr, message, kAppTitle, MB_ICONERROR | MB_OK);
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
    SetTitle(_T("Stany magazynowe"));  // window title, instead of the default "Untitled"
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
