#pragma once

#include "framework.h"
#include "DockPanes.h"

// CFrameWndEx is the Feature Pack frame — it hosts the ribbon, dockable panes and
// honours the active visual manager (theming).
class CMainFrame : public CFrameWndEx {
    DECLARE_DYNCREATE(CMainFrame)

public:
    CMainFrame();

protected:
    afx_msg int OnCreate(LPCREATESTRUCT createStruct);
    afx_msg void OnViewThemeDark();
    afx_msg void OnUpdateViewThemeDark(CCmdUI* cmdUI);
    afx_msg void OnViewPane(UINT cmdId);
    afx_msg void OnUpdateViewPane(CCmdUI* cmdUI);
    DECLARE_MESSAGE_MAP()

private:
    void BuildRibbon();
    void CreatePanes();
    CDockablePane* PaneFor(UINT cmdId);

    CMFCRibbonBar ribbon_;
    CDashboardPane dashboard_;
    CListPane movementLog_;
    CListPane details_;
    bool dark_ = false;
};
