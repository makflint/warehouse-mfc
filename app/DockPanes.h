#pragma once

#include "framework.h"

// Dashboard pane — custom-drawn. Step 3 shows a placeholder; step 4 draws the
// stock bar chart + KPI tiles.
class CDashboardPane : public CDockablePane {
protected:
    afx_msg void OnPaint();
    DECLARE_MESSAGE_MAP()
};

// A dockable pane that hosts a report-style list control (Movement log / Details).
class CListPane : public CDockablePane {
public:
    CListCtrl& List() { return list_; }

protected:
    afx_msg int OnCreate(LPCREATESTRUCT createStruct);
    afx_msg void OnSize(UINT type, int cx, int cy);
    DECLARE_MESSAGE_MAP()

private:
    CListCtrl list_;
};
