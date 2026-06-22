#pragma once

#include <afxvisualmanageroffice2007.h>

// A genuine dark theme: Office 2007 ObsidianBlack gives the dark ribbon/chrome,
// and these overrides darken the list/grid headers (which the stock managers draw
// light by design). Content surfaces (grid bodies, dashboard, property grid) are
// recoloured by the views; see CMainFrame::ApplyContentTheme.
class CDarkVisualManager : public CMFCVisualManagerOffice2007 {
    DECLARE_DYNCREATE(CDarkVisualManager)

public:
    void OnFillHeaderCtrlBackground(CMFCHeaderCtrl* ctrl, CDC* dc, CRect rect) override;
    void OnDrawHeaderCtrlBorder(CMFCHeaderCtrl* ctrl, CDC* dc, CRect& rect, BOOL isPressed,
                               BOOL isHighlighted) override;
    void OnDrawHeaderCtrlSortArrow(CMFCHeaderCtrl* ctrl, CDC* dc, CRect& rect, BOOL isUp) override;
};
