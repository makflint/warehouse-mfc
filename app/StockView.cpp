#include "framework.h"
#include "resource.h"

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
    ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CStockView::OnCustomDraw)
END_MESSAGE_MAP()

namespace {
constexpr int kColWarehouse = 0;
constexpr int kColSku = 1;
constexpr int kColProduct = 2;
constexpr int kColOnHand = 3;
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
        list.InsertColumn(kColWarehouse, _T("Magazyn"), LVCFMT_LEFT, 90);
        list.InsertColumn(kColSku, _T("SKU"), LVCFMT_LEFT, 60);
        list.InsertColumn(kColProduct, _T("Produkt"), LVCFMT_LEFT, 220);
        list.InsertColumn(kColOnHand, _T("Stan"), LVCFMT_RIGHT, 70);
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

    int item = 0;
    const std::vector<warehouse::StockRow>& rows = doc->Stock();
    for (std::size_t idx = 0; idx < rows.size(); ++idx) {
        const warehouse::StockRow& stock = rows[idx];
        if (showLowOnly_ && !stock.isLow) {
            continue;
        }
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

    // Keep the dashboard chart/KPIs in sync with the grid.
    if (auto* frame = DYNAMIC_DOWNCAST(CMainFrame, GetParentFrame())) {
        frame->RefreshPanes();
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
                draw->clrText = RGB(192, 0, 0);
            }
            *result = CDRF_DODEFAULT;
            return;
        }
        default:
            *result = CDRF_DODEFAULT;
            return;
    }
}
