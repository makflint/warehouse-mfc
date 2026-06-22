#pragma once

#include <afxlistctrl.h>
#include <afxpropertygridctrl.h>

#include <vector>

#include "framework.h"
#include "warehouse/stock_repository.hpp"

// Dashboard pane — owner-drawn KPI tiles + bar chart. Follows the dark/light theme
// (it's not a Feature-Pack control, so the manager can't theme it).
class CDashboardPane : public CDockablePane {
public:
    void SetDark(bool dark) {
        dark_ = dark;
        if (GetSafeHwnd() != nullptr) {
            Invalidate(FALSE);
        }
    }

protected:
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT type, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* dc);
    DECLARE_MESSAGE_MAP()

private:
    bool dark_ = false;
};

// Movement-log list: a Feature-Pack list that sorts on the Czas column only
// (the other columns aren't meaningful to sort by).
class CMovementLogList : public CMFCListCtrl {
public:
    void SetRows(const std::vector<warehouse::MovementRow>* rows) { rows_ = rows; }
    void SetDark(bool dark);

protected:
    int OnCompareItems(LPARAM lParam1, LPARAM lParam2, int column) override;
    void Sort(int column, BOOL ascending = TRUE, BOOL add = FALSE) override;
    COLORREF OnGetCellTextColor(int row, int column) override;
    COLORREF OnGetCellBkColor(int row, int column) override;

private:
    const std::vector<warehouse::MovementRow>* rows_ = nullptr;
    bool dark_ = false;
};

// Movement-log pane hosting the list above.
class CListPane : public CDockablePane {
public:
    CMFCListCtrl& List() { return list_; }
    void SetRows(const std::vector<warehouse::MovementRow>* rows) { list_.SetRows(rows); }
    void SetDark(bool dark) { list_.SetDark(dark); }

protected:
    afx_msg int OnCreate(LPCREATESTRUCT createStruct);
    afx_msg void OnSize(UINT type, int cx, int cy);
    DECLARE_MESSAGE_MAP()

private:
    CMovementLogList list_;
    CFont uiFont_;
};

// Details pane built on the Feature-Pack property grid: a themed name/value grid
// (header included), so it follows the visual manager with no manual recolouring.
class CDetailsPane : public CDockablePane {
public:
    void Show(const warehouse::StockRow& row);
    void SetDark(bool dark);

protected:
    afx_msg int OnCreate(LPCREATESTRUCT createStruct);
    afx_msg void OnSize(UINT type, int cx, int cy);
    DECLARE_MESSAGE_MAP()

private:
    CMFCPropertyGridCtrl grid_;
};
