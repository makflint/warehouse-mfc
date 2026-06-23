#pragma once

#include <vector>

#include "framework.h"
#include "warehouse/movement.hpp"
#include "warehouse/stock_repository.hpp"

class CWarehouseDoc;

// The stock grid as a Feature-Pack list: the header is drawn by the visual manager
// (themed look + native sort arrow); low-stock rows stay red and the body follows the
// theme via OnGetCell*Color.
class CStockGrid : public CMFCListCtrl {
public:
    // The current snapshot, indexed by each item's data (its row in Stock()).
    void SetRows(const std::vector<warehouse::StockRow>* rows) { rows_ = rows; }
    void SetDark(bool dark);
    void Resort() {
        if (sortColumn_ >= 0) {
            Sort(sortColumn_, ascending_);
        }
    }

protected:
    int OnCompareItems(LPARAM lParam1, LPARAM lParam2, int column) override;
    COLORREF OnGetCellTextColor(int row, int column) override;
    COLORREF OnGetCellBkColor(int row, int column) override;
    void Sort(int column, BOOL ascending = TRUE, BOOL add = FALSE) override;

private:
    const std::vector<warehouse::StockRow>* rows_ = nullptr;
    bool dark_ = false;
    int sortColumn_ = 0;       // default sort: Magazyn...
    BOOL ascending_ = TRUE;    // ...ascending (matches the SQL ORDER BY); shows the arrow on load
};

// The view hosts the grid (a CView, not a CListView, so it can own a CMFCListCtrl).
class CStockView : public CView {
    DECLARE_DYNCREATE(CStockView)

public:
    CStockView();

    CWarehouseDoc* GetDocument() const;

    void OnInitialUpdate() override;
    void OnUpdate(CView* sender, LPARAM hint, CObject* hintObject) override;
    void OnDraw(CDC* /*dc*/) override {}  // the grid fills the client area

    void SetDark(bool dark) { grid_.SetDark(dark); }

protected:
    afx_msg int OnCreate(LPCREATESTRUCT createStruct);
    afx_msg void OnSize(UINT type, int cx, int cy);
    afx_msg void OnSetFocus(CWnd* old);
    afx_msg void OnStockRefresh();
    afx_msg void OnRecordIn();
    afx_msg void OnRecordOut();
    afx_msg void OnEditUndo();
    afx_msg void OnEditRedo();
    afx_msg void OnUpdateEditUndo(CCmdUI* cmdUI);
    afx_msg void OnUpdateEditRedo(CCmdUI* cmdUI);
    afx_msg void OnFilterLow();
    afx_msg void OnUpdateFilterLow(CCmdUI* cmdUI);
    afx_msg void OnItemChanged(NMHDR* notify, LRESULT* result);
    afx_msg void OnGridDblClk(NMHDR* notify, LRESULT* result);
    afx_msg void OnGridRClick(NMHDR* notify, LRESULT* result);
    DECLARE_MESSAGE_MAP()

private:
    void RecordMovement(warehouse::MovementType type);
    void ShowLowOnly(bool on);

    CStockGrid grid_;
    CFont uiFont_;
    bool showLowOnly_ = false;
};
