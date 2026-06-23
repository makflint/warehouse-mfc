#include "framework.h"
#include "resource.h"

#include <set>
#include <string>
#include <vector>

#include "MainFrame.h"
#include "RecordMovementDialog.h"
#include "I18n.h"
#include "StockView.h"
#include "TextUtil.h"
#include "WarehouseDoc.h"
#include "warehouse/view_logic.hpp"

namespace {
constexpr int kColWarehouse = 0;
constexpr int kColSku = 1;
constexpr int kColProduct = 2;
constexpr int kColOnHand = 3;
constexpr UINT kGridId = 1;  // child id of the embedded grid
}  // namespace

// --- CStockGrid (Feature-Pack list) ----------------------------------------

// Items carry their Stock() row index as item data, so a click-sort compares the
// underlying fields (numeric for Stan, text otherwise) rather than the cell strings.
int CStockGrid::OnCompareItems(LPARAM lParam1, LPARAM lParam2, int column) {
    if (rows_ == nullptr) {
        return 0;
    }
    const std::size_t a = static_cast<std::size_t>(lParam1);
    const std::size_t b = static_cast<std::size_t>(lParam2);
    if (a >= rows_->size() || b >= rows_->size()) {
        return 0;
    }
    const warehouse::StockRow& ra = (*rows_)[a];
    const warehouse::StockRow& rb = (*rows_)[b];
    return warehouse::compareStock(ra, rb, static_cast<warehouse::StockColumn>(column));
}

COLORREF CStockGrid::OnGetCellTextColor(int row, int column) {
    if (rows_ != nullptr) {
        const std::size_t idx = GetItemData(row);
        if (idx < rows_->size() && (*rows_)[idx].isLow) {
            return ThemeColorsFor(dark_).lowStock;  // low-stock rows stay red
        }
    }
    return dark_ ? ThemeColorsFor(true).text : CMFCListCtrl::OnGetCellTextColor(row, column);
}

COLORREF CStockGrid::OnGetCellBkColor(int row, int column) {
    return dark_ ? ThemeColorsFor(true).background : CMFCListCtrl::OnGetCellBkColor(row, column);
}

void CStockGrid::SetDark(bool dark) {
    dark_ = dark;
    if (GetSafeHwnd() == nullptr) {
        return;
    }
    // Colour the whole control (incl. the empty area below the rows), not just cells.
    const COLORREF bg = dark ? ThemeColorsFor(true).background : ::GetSysColor(COLOR_WINDOW);
    SetBkColor(bg);
    SetTextBkColor(bg);
    // Grid lines look busy on dark (they run through the empty rows); drop them in
    // dark to match the clean property grid, keep them on light for scanning.
    const DWORD ex = GetExtendedStyle();
    SetExtendedStyle(dark ? (ex & ~LVS_EX_GRIDLINES) : (ex | LVS_EX_GRIDLINES));
    // The light sort-column mark leaves a stray light strip on dark; the header
    // arrow already indicates the sort, so disable the mark in dark.
    EnableMarkSortedColumn(!dark, FALSE);
    Invalidate();
}

// Override so the active sort column/direction is captured (native header click or
// our own Resort()), letting us re-apply it after the data reloads.
void CStockGrid::Sort(int column, BOOL ascending, BOOL add) {
    sortColumn_ = column;
    ascending_ = ascending;
    CMFCListCtrl::Sort(column, ascending, add);
}

// --- CStockView ------------------------------------------------------------

IMPLEMENT_DYNCREATE(CStockView, CView)

BEGIN_MESSAGE_MAP(CStockView, CView)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_SETFOCUS()
    ON_COMMAND(ID_STOCK_REFRESH, &CStockView::OnStockRefresh)
    ON_COMMAND(ID_STOCK_RECORD_IN, &CStockView::OnRecordIn)
    ON_COMMAND(ID_STOCK_RECORD_OUT, &CStockView::OnRecordOut)
    ON_COMMAND(ID_EDIT_UNDO, &CStockView::OnEditUndo)
    ON_COMMAND(ID_EDIT_REDO, &CStockView::OnEditRedo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, &CStockView::OnUpdateEditUndo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, &CStockView::OnUpdateEditRedo)
    ON_COMMAND(ID_STOCK_FILTER_LOW, &CStockView::OnFilterLow)
    ON_UPDATE_COMMAND_UI(ID_STOCK_FILTER_LOW, &CStockView::OnUpdateFilterLow)
    ON_NOTIFY(LVN_ITEMCHANGED, kGridId, &CStockView::OnItemChanged)
    ON_NOTIFY(NM_DBLCLK, kGridId, &CStockView::OnGridDblClk)
    ON_NOTIFY(NM_RCLICK, kGridId, &CStockView::OnGridRClick)
END_MESSAGE_MAP()

CStockView::CStockView() {}

CWarehouseDoc* CStockView::GetDocument() const {
    return static_cast<CWarehouseDoc*>(m_pDocument);
}

int CStockView::OnCreate(LPCREATESTRUCT createStruct) {
    if (CView::OnCreate(createStruct) == -1) {
        return -1;
    }
    const DWORD style = WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS;
    grid_.Create(style, CRect(0, 0, 0, 0), this, kGridId);
    grid_.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    grid_.EnableMarkSortedColumn(TRUE);  // highlight + arrow on the sorted column
    grid_.InsertColumn(kColWarehouse, i18n::T(i18n::ColWarehouse), LVCFMT_LEFT, 105);  // fits EN "Warehouse" + sort arrow
    grid_.InsertColumn(kColSku, i18n::T(i18n::ColSku), LVCFMT_LEFT, 70);
    grid_.InsertColumn(kColProduct, i18n::T(i18n::ColProduct), LVCFMT_LEFT, 220);
    grid_.InsertColumn(kColOnHand, i18n::T(i18n::ColOnHand), LVCFMT_RIGHT, 70);
    CreateUiFont(uiFont_);
    if (uiFont_.GetSafeHandle() != nullptr) {
        grid_.SetFont(&uiFont_);
    }
    return 0;
}

void CStockView::OnSize(UINT type, int cx, int cy) {
    CView::OnSize(type, cx, cy);
    if (grid_.GetSafeHwnd() != nullptr) {
        grid_.SetWindowPos(nullptr, 0, 0, cx, cy, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void CStockView::OnSetFocus(CWnd* /*old*/) {
    if (grid_.GetSafeHwnd() != nullptr) {
        grid_.SetFocus();  // keep keyboard focus on the grid, not the bare view
    }
}

void CStockView::OnInitialUpdate() {
    CView::OnInitialUpdate();  // triggers OnUpdate to fill the rows
    if (grid_.GetItemCount() > 0) {  // select the first row so the Details pane fills
        grid_.SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
}

void CStockView::OnUpdate(CView* /*sender*/, LPARAM /*hint*/, CObject* /*hintObject*/) {
    const CWarehouseDoc* doc = GetDocument();
    if (doc == nullptr || grid_.GetSafeHwnd() == nullptr) {
        return;
    }
    const std::vector<warehouse::StockRow>& rows = doc->Stock();
    grid_.SetRows(&rows);
    grid_.DeleteAllItems();

    int item = 0;
    for (std::size_t idx = 0; idx < rows.size(); ++idx) {
        const warehouse::StockRow& stock = rows[idx];
        if (showLowOnly_ && !stock.isLow) {
            continue;
        }
        grid_.InsertItem(item, FromUtf8(stock.warehouseCode));
        grid_.SetItemText(item, kColSku, FromUtf8(stock.sku));
        grid_.SetItemText(item, kColProduct, FromUtf8(stock.productName));
        CString onHand;
        onHand.Format(_T("%d"), stock.onHand);
        grid_.SetItemText(item, kColOnHand, onHand);
        // Item data = index into Stock(): drives sort comparison, row colour, Details.
        grid_.SetItemData(item, static_cast<DWORD_PTR>(idx));
        ++item;
    }
    grid_.Resort();  // keep the active sort after a data reload

    // Keep the dashboard chart/KPIs in sync with the grid, and the status-bar row count
    // in sync with what's actually shown (visible vs total when filtered).
    if (auto* frame = DYNAMIC_DOWNCAST(CMainFrame, GetParentFrame())) {
        frame->RefreshPanes();
        frame->SetRowCount(grid_.GetItemCount(), static_cast<int>(rows.size()), showLowOnly_);
    }
}

void CStockView::OnStockRefresh() {
    if (CWarehouseDoc* doc = GetDocument()) {
        doc->Refresh();
    }
}

void CStockView::OnRecordIn() { RecordMovement(warehouse::MovementType::In); }
void CStockView::OnRecordOut() { RecordMovement(warehouse::MovementType::Out); }

void CStockView::RecordMovement(warehouse::MovementType type) {
    CWarehouseDoc* doc = GetDocument();
    if (doc == nullptr) {
        return;
    }

    // Distinct products and warehouses for the dialog combos, taken from the data.
    std::vector<ComboOption> products;
    std::vector<ComboOption> warehouses;
    std::set<int> seenProducts;
    std::set<int> seenWarehouses;
    for (const warehouse::StockRow& row : doc->Stock()) {
        if (seenProducts.insert(row.productId).second) {
            products.push_back({row.productId, FromUtf8(row.sku + " " + row.productName)});
        }
        if (seenWarehouses.insert(row.warehouseId).second) {
            warehouses.push_back({row.warehouseId, FromUtf8(row.warehouseCode + " " + row.warehouseName)});
        }
    }

    // Pre-select the product/warehouse from the grid's current row.
    int selProduct = 0;
    int selWarehouse = 0;
    const int selected = grid_.GetNextItem(-1, LVNI_SELECTED);
    if (selected >= 0) {
        const std::size_t idx = grid_.GetItemData(selected);
        if (idx < doc->Stock().size()) {
            selProduct = doc->Stock()[idx].productId;
            selWarehouse = doc->Stock()[idx].warehouseId;
        }
    }

    CRecordMovementDialog dialog(type, std::move(products), std::move(warehouses), this);
    dialog.Preselect(selProduct, selWarehouse);
    if (dialog.DoModal() == IDOK) {
        doc->ExecuteMovement(
            warehouse::Movement{dialog.productId(), dialog.warehouseId(), dialog.qty(), type});
    }
}

void CStockView::OnEditUndo() {
    if (CWarehouseDoc* doc = GetDocument()) {
        doc->Undo();
    }
}

void CStockView::OnEditRedo() {
    if (CWarehouseDoc* doc = GetDocument()) {
        doc->Redo();
    }
}

void CStockView::OnUpdateEditUndo(CCmdUI* cmdUI) {
    const CWarehouseDoc* doc = GetDocument();
    cmdUI->Enable(doc != nullptr && doc->CanUndo());
}

void CStockView::OnUpdateEditRedo(CCmdUI* cmdUI) {
    const CWarehouseDoc* doc = GetDocument();
    cmdUI->Enable(doc != nullptr && doc->CanRedo());
}

void CStockView::OnFilterLow() { ShowLowOnly(!showLowOnly_); }

void CStockView::ShowLowOnly(bool on) {
    showLowOnly_ = on;
    OnUpdate(nullptr, 0, nullptr);  // re-populate with the filter applied
}

void CStockView::OnUpdateFilterLow(CCmdUI* cmdUI) {
    cmdUI->SetCheck(showLowOnly_ ? 1 : 0);
}

// On row selection, push that product's details into the Details pane.
void CStockView::OnItemChanged(NMHDR* notify, LRESULT* result) {
    auto* nm = reinterpret_cast<NMLISTVIEW*>(notify);
    *result = 0;
    const bool becameSelected = (nm->uChanged & LVIF_STATE) && (nm->uNewState & LVIS_SELECTED);
    if (!becameSelected || nm->iItem < 0) {
        return;
    }
    const CWarehouseDoc* doc = GetDocument();
    if (doc == nullptr) {
        return;
    }
    const std::size_t idx = grid_.GetItemData(nm->iItem);
    if (idx < doc->Stock().size()) {
        if (auto* frame = DYNAMIC_DOWNCAST(CMainFrame, GetParentFrame())) {
            frame->ShowDetails(doc->Stock()[idx]);
        }
    }
}

// Double-click a row → record a movement for it (Przyjmij, pre-selected to that product).
void CStockView::OnGridDblClk(NMHDR* /*notify*/, LRESULT* result) {
    *result = 0;
    if (grid_.GetNextItem(-1, LVNI_SELECTED) >= 0) {
        RecordMovement(warehouse::MovementType::In);
    }
}

// Right-click a row → select it and show a context menu of the row actions. The command ids
// match the ribbon, so they route through the frame to the existing handlers.
void CStockView::OnGridRClick(NMHDR* notify, LRESULT* result) {
    *result = 0;
    const auto* item = reinterpret_cast<NMITEMACTIVATE*>(notify);
    if (item->iItem >= 0) {
        grid_.SetItemState(item->iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, ID_STOCK_RECORD_IN, i18n::T(i18n::BtnReceive));
    menu.AppendMenu(MF_STRING, ID_STOCK_RECORD_OUT, i18n::T(i18n::BtnIssue));
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, ID_STOCK_REFRESH, i18n::T(i18n::BtnRefresh));
    menu.AppendMenu(MF_STRING, ID_TOGGLE_DETAILS, i18n::T(i18n::BtnDetails));
    CPoint pt;
    ::GetCursorPos(&pt);
    menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, AfxGetMainWnd());
}
