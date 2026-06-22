#include "framework.h"
#include "resource.h"

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include "MainFrame.h"
#include "RecordMovementDialog.h"
#include "StockView.h"
#include "TextUtil.h"
#include "WarehouseDoc.h"

IMPLEMENT_DYNCREATE(CStockView, CListView)

BEGIN_MESSAGE_MAP(CStockView, CListView)
    ON_COMMAND(ID_STOCK_REFRESH, &CStockView::OnStockRefresh)
    ON_COMMAND(ID_STOCK_RECORD_IN, &CStockView::OnRecordIn)
    ON_COMMAND(ID_STOCK_RECORD_OUT, &CStockView::OnRecordOut)
    ON_COMMAND(ID_EDIT_UNDO, &CStockView::OnEditUndo)
    ON_COMMAND(ID_EDIT_REDO, &CStockView::OnEditRedo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, &CStockView::OnUpdateEditUndo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, &CStockView::OnUpdateEditRedo)
    ON_COMMAND(ID_STOCK_FILTER_LOW, &CStockView::OnFilterLow)
    ON_UPDATE_COMMAND_UI(ID_STOCK_FILTER_LOW, &CStockView::OnUpdateFilterLow)
    ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CStockView::OnItemChanged)
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CStockView::OnColumnClick)
    ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CStockView::OnCustomDraw)
END_MESSAGE_MAP()

namespace {
constexpr int kColWarehouse = 0;
constexpr int kColSku = 1;
constexpr int kColProduct = 2;
constexpr int kColOnHand = 3;
constexpr int kColumnCount = 4;
const LPCTSTR kColumnTitles[kColumnCount] = {_T("Magazyn"), _T("Symbol"), _T("Produkt"), _T("Stan")};
}  // namespace

CStockView::CStockView() {}

CWarehouseDoc* CStockView::GetDocument() const {
    return static_cast<CWarehouseDoc*>(m_pDocument);
}

BOOL CStockView::PreCreateWindow(CREATESTRUCT& cs) {
    cs.style |= LVS_REPORT;
    return CListView::PreCreateWindow(cs);
}

void CStockView::OnInitialUpdate() {
    CListCtrl& list = GetListCtrl();
    if (list.GetHeaderCtrl()->GetItemCount() == 0) {
        list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        list.InsertColumn(kColWarehouse, kColumnTitles[kColWarehouse], LVCFMT_LEFT, 90);
        list.InsertColumn(kColSku, kColumnTitles[kColSku], LVCFMT_LEFT, 70);
        list.InsertColumn(kColProduct, kColumnTitles[kColProduct], LVCFMT_LEFT, 220);
        list.InsertColumn(kColOnHand, kColumnTitles[kColOnHand], LVCFMT_RIGHT, 70);
        CreateUiFont(uiFont_);  // modern UI font, matching the dock-pane lists
        if (uiFont_.GetSafeHandle() != nullptr) {
            list.SetFont(&uiFont_);
        }
    }
    CListView::OnInitialUpdate();  // triggers OnUpdate to fill the rows
    if (list.GetItemCount() > 0) {  // select the first row so the Details pane is populated
        list.SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
}

void CStockView::OnUpdate(CView* /*sender*/, LPARAM /*hint*/, CObject* /*hintObject*/) {
    const CWarehouseDoc* doc = GetDocument();
    if (doc == nullptr) {
        return;
    }
    CListCtrl& list = GetListCtrl();
    list.DeleteAllItems();

    const std::vector<warehouse::StockRow>& rows = doc->Stock();

    // Build the display order (filtered, then sorted by the chosen column).
    std::vector<std::size_t> order;
    order.reserve(rows.size());
    for (std::size_t idx = 0; idx < rows.size(); ++idx) {
        if (!showLowOnly_ || rows[idx].isLow) {
            order.push_back(idx);
        }
    }
    if (sortColumn_ >= 0) {
        std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
            const warehouse::StockRow& ra = rows[a];
            const warehouse::StockRow& rb = rows[b];
            int cmp = 0;
            switch (sortColumn_) {
                case kColWarehouse: cmp = ra.warehouseCode.compare(rb.warehouseCode); break;
                case kColSku:       cmp = ra.sku.compare(rb.sku); break;
                case kColProduct:   cmp = ra.productName.compare(rb.productName); break;
                case kColOnHand:    cmp = ra.onHand - rb.onHand; break;
            }
            return sortAscending_ ? cmp < 0 : cmp > 0;
        });
    }

    int item = 0;
    for (std::size_t idx : order) {
        const warehouse::StockRow& stock = rows[idx];
        list.InsertItem(item, FromUtf8(stock.warehouseCode));
        list.SetItemText(item, kColSku, FromUtf8(stock.sku));
        list.SetItemText(item, kColProduct, FromUtf8(stock.productName));
        CString onHand;
        onHand.Format(_T("%d"), stock.onHand);
        list.SetItemText(item, kColOnHand, onHand);
        // Item data = index into Stock(): drives the red custom-draw and the Details pane.
        list.SetItemData(item, static_cast<DWORD_PTR>(idx));
        ++item;
    }
    UpdateSortArrow();

    // Keep the dashboard chart/KPIs in sync with the grid.
    if (auto* frame = DYNAMIC_DOWNCAST(CMainFrame, GetParentFrame())) {
        frame->RefreshPanes();
    }
}

// Click a header to sort by that column; click again to flip the direction.
void CStockView::OnColumnClick(NMHDR* notify, LRESULT* result) {
    const int column = reinterpret_cast<NMLISTVIEW*>(notify)->iSubItem;
    if (column == sortColumn_) {
        sortAscending_ = !sortAscending_;
    } else {
        sortColumn_ = column;
        sortAscending_ = true;
    }
    OnUpdate(nullptr, 0, nullptr);  // re-populate in the new order
    *result = 0;
}

// Show a ▲/▼ glyph next to the active column's title. (Appending to the header
// text is more reliable across themes than the native HDF_SORTUP header flag.)
void CStockView::UpdateSortArrow() {
    CListCtrl& list = GetListCtrl();
    if (list.GetHeaderCtrl() == nullptr) {
        return;
    }
    for (int i = 0; i < kColumnCount; ++i) {
        CString title = kColumnTitles[i];
        if (i == sortColumn_) {
            title += sortAscending_ ? _T("  \x25B2") : _T("  \x25BC");  // ▲ / ▼
        }
        LVCOLUMN column = {};
        column.mask = LVCF_TEXT;
        column.pszText = const_cast<LPTSTR>(static_cast<LPCTSTR>(title));
        list.SetColumn(i, &column);
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

    CRecordMovementDialog dialog(type, std::move(products), std::move(warehouses), this);
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

void CStockView::SetDark(bool dark) {
    dark_ = dark;
    ApplyListTheme(GetListCtrl(), dark);  // bg/text follow the theme; row colours via custom-draw
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
    const std::size_t idx = GetListCtrl().GetItemData(nm->iItem);
    if (idx < doc->Stock().size()) {
        if (auto* frame = DYNAMIC_DOWNCAST(CMainFrame, GetParentFrame())) {
            frame->ShowDetails(doc->Stock()[idx]);
        }
    }
}

// Paint low-stock rows in red (OnHand <= ReorderLevel, per the IsLow flag).
void CStockView::OnCustomDraw(NMHDR* notify, LRESULT* result) {
    auto* draw = reinterpret_cast<NMLVCUSTOMDRAW*>(notify);
    switch (draw->nmcd.dwDrawStage) {
        case CDDS_PREPAINT:
            *result = CDRF_NOTIFYITEMDRAW;
            return;
        case CDDS_ITEMPREPAINT: {
            const int row = static_cast<int>(draw->nmcd.dwItemSpec);
            const CWarehouseDoc* doc = GetDocument();
            const std::size_t idx = GetListCtrl().GetItemData(row);
            if (doc != nullptr && idx < doc->Stock().size() && doc->Stock()[idx].isLow) {
                draw->clrText = ThemeColorsFor(dark_).lowStock;  // brighter red on dark
            }
            *result = CDRF_DODEFAULT;
            return;
        }
        default:
            *result = CDRF_DODEFAULT;
            return;
    }
}
