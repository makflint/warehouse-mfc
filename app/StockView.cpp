#include "framework.h"
#include "resource.h"

#include "StockView.h"

IMPLEMENT_DYNCREATE(CStockView, CListView)

BEGIN_MESSAGE_MAP(CStockView, CListView)
END_MESSAGE_MAP()

CStockView::CStockView() {}

BOOL CStockView::PreCreateWindow(CREATESTRUCT& cs) {
    cs.style |= LVS_REPORT;
    return CListView::PreCreateWindow(cs);
}

void CStockView::OnInitialUpdate() {
    CListView::OnInitialUpdate();

    CListCtrl& list = GetListCtrl();
    list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    list.InsertColumn(0, _T("Magazyn"), LVCFMT_LEFT, 90);
    list.InsertColumn(1, _T("SKU"), LVCFMT_LEFT, 60);
    list.InsertColumn(2, _T("Produkt"), LVCFMT_LEFT, 220);
    list.InsertColumn(3, _T("Stan"), LVCFMT_RIGHT, 70);
}
