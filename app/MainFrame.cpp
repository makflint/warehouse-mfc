#include "framework.h"
#include "resource.h"

#include "MainFrame.h"

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
    ON_WM_CREATE()
    ON_COMMAND(ID_VIEW_THEME_DARK, &CMainFrame::OnViewThemeDark)
    ON_UPDATE_COMMAND_UI(ID_VIEW_THEME_DARK, &CMainFrame::OnUpdateViewThemeDark)
END_MESSAGE_MAP()

CMainFrame::CMainFrame() {}

int CMainFrame::OnCreate(LPCREATESTRUCT createStruct) {
    if (CFrameWndEx::OnCreate(createStruct) == -1) {
        return -1;
    }
    BuildRibbon();
    SetMenu(nullptr);  // ribbon replaces the classic menu (kept only as the SDI shared menu)
    return 0;
}

// The ribbon replaces the classic menu. Buttons carry the same command IDs the view
// already handles, so routing (and undo/redo enable state) works unchanged.
void CMainFrame::BuildRibbon() {
    ribbon_.Create(this);
    ribbon_.EnableToolTips();

    CMFCRibbonCategory* magazyn = ribbon_.AddCategory(_T("Magazyn"), 0, 0);

    CMFCRibbonPanel* stany = magazyn->AddPanel(_T("Stany"));
    stany->Add(new CMFCRibbonButton(ID_STOCK_REFRESH, _T("Odśwież")));
    stany->Add(new CMFCRibbonButton(ID_STOCK_FILTER_LOW, _T("Tylko niskie")));

    CMFCRibbonPanel* ruchy = magazyn->AddPanel(_T("Ruchy"));
    ruchy->Add(new CMFCRibbonButton(ID_STOCK_RECORD_IN, _T("Przyjmij")));
    ruchy->Add(new CMFCRibbonButton(ID_STOCK_RECORD_OUT, _T("Wydaj")));

    CMFCRibbonPanel* edycja = magazyn->AddPanel(_T("Edycja"));
    edycja->Add(new CMFCRibbonButton(ID_EDIT_UNDO, _T("Cofnij")));
    edycja->Add(new CMFCRibbonButton(ID_EDIT_REDO, _T("Ponów")));

    CMFCRibbonCategory* widok = ribbon_.AddCategory(_T("Widok"), 0, 0);
    CMFCRibbonPanel* motyw = widok->AddPanel(_T("Motyw"));
    motyw->Add(new CMFCRibbonButton(ID_VIEW_THEME_DARK, _T("Ciemny motyw")));
}

void CMainFrame::OnViewThemeDark() {
    dark_ = !dark_;
    CMFCVisualManagerOffice2007::SetStyle(dark_ ? CMFCVisualManagerOffice2007::Office2007_ObsidianBlack
                                                : CMFCVisualManagerOffice2007::Office2007_LunaBlue);
    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
    RedrawWindow(nullptr, nullptr, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);
}

void CMainFrame::OnUpdateViewThemeDark(CCmdUI* cmdUI) {
    cmdUI->SetCheck(dark_ ? 1 : 0);
}
