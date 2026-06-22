#pragma once

#include "framework.h"
#include "DockPanes.h"

namespace warehouse {
struct StockRow;
}

// CFrameWndEx is the Feature Pack frame — it hosts the ribbon, dockable panes and
// honours the active visual manager (theming).
class CMainFrame : public CFrameWndEx {
    DECLARE_DYNCREATE(CMainFrame)

public:
    CMainFrame();

    // Repaint/refresh the docking panes after the stock snapshot changed.
    void RefreshPanes();

    // Fill the Details pane with the selected stock row.
    void ShowDetails(const warehouse::StockRow& row);

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

    void CreateStatusBar();
    void SetStatusPane(UINT id, const CString& text);

    CMFCRibbonBar ribbon_;
    CMFCRibbonStatusBar statusBar_;
    CDashboardPane dashboard_;
    CListPane movementLog_;
    CListPane details_;
    bool dark_ = false;
};
