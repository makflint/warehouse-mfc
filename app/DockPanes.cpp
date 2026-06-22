#include "framework.h"

#include "DockPanes.h"

// --- Dashboard pane --------------------------------------------------------
BEGIN_MESSAGE_MAP(CDashboardPane, CDockablePane)
    ON_WM_PAINT()
END_MESSAGE_MAP()

void CDashboardPane::OnPaint() {
    CPaintDC dc(this);
    CRect rc;
    GetClientRect(rc);
    dc.FillSolidRect(rc, ::GetSysColor(COLOR_WINDOW));
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
    dc.DrawText(_T("Pulpit — wykres stanów (wkrótce)"), rc,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// --- List pane (Movement log / Details) ------------------------------------
BEGIN_MESSAGE_MAP(CListPane, CDockablePane)
    ON_WM_CREATE()
    ON_WM_SIZE()
END_MESSAGE_MAP()

int CListPane::OnCreate(LPCREATESTRUCT createStruct) {
    if (CDockablePane::OnCreate(createStruct) == -1) {
        return -1;
    }
    const DWORD style = WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS;
    list_.Create(style, CRect(0, 0, 0, 0), this, 1);
    list_.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    return 0;
}

void CListPane::OnSize(UINT type, int cx, int cy) {
    CDockablePane::OnSize(type, cx, cy);
    if (list_.GetSafeHwnd() != nullptr) {
        list_.SetWindowPos(nullptr, 0, 0, cx, cy, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}
