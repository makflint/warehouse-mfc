#pragma once

#include "framework.h"

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
    DECLARE_MESSAGE_MAP()

private:
    void BuildRibbon();

    CMFCRibbonBar ribbon_;
    bool dark_ = false;
};
