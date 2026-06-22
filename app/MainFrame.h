#pragma once

#include "framework.h"
#include "resource.h"
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
    afx_msg void OnTheme(UINT cmdId);
    afx_msg void OnUpdateTheme(CCmdUI* cmdUI);
    afx_msg void OnUpdateThemeMenu(CCmdUI* cmdUI);
    afx_msg void OnViewPane(UINT cmdId);
    afx_msg void OnUpdateViewPane(CCmdUI* cmdUI);
    DECLARE_MESSAGE_MAP()

private:
    void BuildRibbon();
    void CreatePanes();
    CDockablePane* PaneFor(UINT cmdId);

    void CreateStatusBar();
    void SetStatusPane(UINT id, const CString& text);
    void ApplyContentTheme(bool dark);  // recolour the non-Feature-Pack content

    CMFCRibbonBar ribbon_;
    CMFCRibbonStatusBar statusBar_;
    CDashboardPane dashboard_;
    CListPane movementLog_;
    CDetailsPane details_;
    UINT currentTheme_ = ID_THEME_OFFICE_BLUE;  // app starts in Office 2007 LunaBlue
};
