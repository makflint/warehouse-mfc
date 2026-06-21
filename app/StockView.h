#pragma once

#include "framework.h"

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

protected:
    DECLARE_MESSAGE_MAP()
};
