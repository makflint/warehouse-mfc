#pragma once

#include "framework.h"
#include "warehouse/movement.hpp"

class CWarehouseDoc;

// The stock grid: a report-style list showing vCurrentStock.
class CStockView : public CListView {
    DECLARE_DYNCREATE(CStockView)

public:
    CStockView();

    CWarehouseDoc* GetDocument() const;

    BOOL PreCreateWindow(CREATESTRUCT& cs) override;
    void OnInitialUpdate() override;
    void OnUpdate(CView* sender, LPARAM hint, CObject* hintObject) override;

    // Recolour the grid for the dark/light theme (driven by the main frame).
    void SetDark(bool dark);

protected:
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
    afx_msg void OnColumnClick(NMHDR* notify, LRESULT* result);
    afx_msg void OnCustomDraw(NMHDR* notify, LRESULT* result);
    DECLARE_MESSAGE_MAP()

private:
    void RecordMovement(warehouse::MovementType type);
    void ShowLowOnly(bool on);
    void UpdateSortArrow();

    bool showLowOnly_ = false;
    bool dark_ = false;
    int sortColumn_ = -1;  // -1 = database order; otherwise the sorted column
    bool sortAscending_ = true;
    CFont uiFont_;
};
