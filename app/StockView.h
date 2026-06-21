#pragma once

#include "framework.h"

// The stock grid: a report-style list showing vCurrentStock.
class CStockView : public CListView {
    DECLARE_DYNCREATE(CStockView)

public:
    CStockView();

    BOOL PreCreateWindow(CREATESTRUCT& cs) override;
    void OnInitialUpdate() override;

protected:
    DECLARE_MESSAGE_MAP()
};
