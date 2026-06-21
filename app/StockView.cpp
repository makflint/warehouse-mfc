#include "framework.h"
#include "resource.h"

#include "StockView.h"
#include "TextUtil.h"
#include "WarehouseDoc.h"

IMPLEMENT_DYNCREATE(CStockView, CListView)

BEGIN_MESSAGE_MAP(CStockView, CListView)
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
