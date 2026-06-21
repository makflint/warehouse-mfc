#include "framework.h"
#include "resource.h"

#include <set>

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
}

void CStockView::OnUpdate(CView* /*sender*/, LPARAM /*hint*/, CObject* /*hintObject*/) {
    const CWarehouseDoc* doc = GetDocument();
    if (doc == nullptr) {
        return;
    }
    CListCtrl& list = GetListCtrl();
    list.DeleteAllItems();

    int row = 0;
    for (const warehouse::StockRow& stock : doc->Stock()) {
        list.InsertItem(row, FromUtf8(stock.warehouseCode));
        list.SetItemText(row, kColSku, FromUtf8(stock.sku));
        list.SetItemText(row, kColProduct, FromUtf8(stock.productName));
        CString onHand;
        onHand.Format(_T("%d"), stock.onHand);
        list.SetItemText(row, kColOnHand, onHand);
        ++row;
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

// Paint low-stock rows in red (OnHand <= ReorderLevel, per the IsLow flag).
void CStockView::OnCustomDraw(NMHDR* notify, LRESULT* result) {
    auto* draw = reinterpret_cast<NMLVCUSTOMDRAW*>(notify);
    switch (draw->nmcd.dwDrawStage) {
        case CDDS_PREPAINT:
            *result = CDRF_NOTIFYITEMDRAW;
            return;
        case CDDS_ITEMPREPAINT: {
            const CWarehouseDoc* doc = GetDocument();
            const int row = static_cast<int>(draw->nmcd.dwItemSpec);
            if (doc != nullptr && row >= 0 && row < static_cast<int>(doc->Stock().size()) &&
                doc->Stock()[row].isLow) {
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
