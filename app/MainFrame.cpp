#include "framework.h"
#include "resource.h"

#include "MainFrame.h"
#include "TextUtil.h"
#include "WarehouseDoc.h"
#include "warehouse/stock_repository.hpp"

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
    ON_WM_CREATE()
    ON_COMMAND(ID_VIEW_THEME_DARK, &CMainFrame::OnViewThemeDark)
    ON_UPDATE_COMMAND_UI(ID_VIEW_THEME_DARK, &CMainFrame::OnUpdateViewThemeDark)
    ON_COMMAND_RANGE(ID_TOGGLE_DASHBOARD, ID_TOGGLE_DETAILS, &CMainFrame::OnViewPane)
    ON_UPDATE_COMMAND_UI_RANGE(ID_TOGGLE_DASHBOARD, ID_TOGGLE_DETAILS, &CMainFrame::OnUpdateViewPane)
END_MESSAGE_MAP()

CMainFrame::CMainFrame() {}

int CMainFrame::OnCreate(LPCREATESTRUCT createStruct) {
    if (CFrameWndEx::OnCreate(createStruct) == -1) {
        return -1;
    }
    BuildRibbon();
    SetMenu(nullptr);  // ribbon replaces the classic menu (kept only as the SDI shared menu)
    CreateStatusBar();  // reserve the bottom edge before the docking area is laid out
    CreatePanes();
    return 0;
}

// The ribbon status bar themes with the ribbon. Row count + selected SKU sit on
// the left; the connection profile is right-aligned as an "extended" element. The
// label strings double as the pane's sizing hint.
void CMainFrame::CreateStatusBar() {
    statusBar_.Create(this);
    statusBar_.AddElement(new CMFCRibbonStatusBarPane(ID_INDICATOR_ROWS, _T("Indeksy: 0000"), TRUE), _T(""));
    statusBar_.AddElement(new CMFCRibbonStatusBarPane(ID_INDICATOR_SKU, _T("SKU: —"), TRUE), _T(""));
    statusBar_.AddExtendedElement(new CMFCRibbonStatusBarPane(ID_INDICATOR_DB, _T("DEMO · LocalDB"), TRUE), _T(""));
}

void CMainFrame::SetStatusPane(UINT id, const CString& text) {
    if (auto* pane = DYNAMIC_DOWNCAST(CMFCRibbonStatusBarPane, statusBar_.FindByID(id))) {
        pane->SetText(text);
        statusBar_.RecalcLayout();
    }
}

// The ribbon replaces the classic menu. Buttons carry the same command IDs the view
// already handles, so routing (and undo/redo enable state) works unchanged.
void CMainFrame::BuildRibbon() {
    ribbon_.Create(this);
    ribbon_.EnableToolTips();

    // Each category owns a small (16px) + large (32px) image strip; the button's
    // image index selects its glyph within that category's strip.
    CMFCRibbonCategory* magazyn =
        ribbon_.AddCategory(_T("Magazyn"), IDB_RIBBON_MAGAZYN_16, IDB_RIBBON_MAGAZYN_32);

    CMFCRibbonPanel* stany = magazyn->AddPanel(_T("Stany"));
    stany->Add(new CMFCRibbonButton(ID_STOCK_REFRESH, _T("Odśwież"), 0, 0));
    stany->Add(new CMFCRibbonButton(ID_STOCK_FILTER_LOW, _T("Tylko niskie"), 1, 1));

    CMFCRibbonPanel* ruchy = magazyn->AddPanel(_T("Ruchy"));
    ruchy->Add(new CMFCRibbonButton(ID_STOCK_RECORD_IN, _T("Przyjmij"), 2, 2));
    ruchy->Add(new CMFCRibbonButton(ID_STOCK_RECORD_OUT, _T("Wydaj"), 3, 3));

    CMFCRibbonPanel* edycja = magazyn->AddPanel(_T("Edycja"));
    edycja->Add(new CMFCRibbonButton(ID_EDIT_UNDO, _T("Cofnij"), 4, 4));
    edycja->Add(new CMFCRibbonButton(ID_EDIT_REDO, _T("Ponów"), 5, 5));

    CMFCRibbonCategory* widok =
        ribbon_.AddCategory(_T("Widok"), IDB_RIBBON_WIDOK_16, IDB_RIBBON_WIDOK_32);
    CMFCRibbonPanel* motyw = widok->AddPanel(_T("Motyw"));
    motyw->Add(new CMFCRibbonButton(ID_VIEW_THEME_DARK, _T("Ciemny motyw"), 0, 0));

    CMFCRibbonPanel* panele = widok->AddPanel(_T("Panele"));
    panele->Add(new CMFCRibbonButton(ID_TOGGLE_DASHBOARD, _T("Pulpit"), 1, 1));
    panele->Add(new CMFCRibbonButton(ID_TOGGLE_MOVEMENTS, _T("Dziennik"), 2, 2));
    panele->Add(new CMFCRibbonButton(ID_TOGGLE_DETAILS, _T("Szczegóły"), 3, 3));
}

// Three docking panels managed by CFrameWndEx: a (custom-drawn) Dashboard on the
// left, and the Movement-log + Details list panes on the right.
void CMainFrame::CreatePanes() {
    CDockingManager::SetDockingMode(DT_SMART);
    EnableDocking(CBRS_ALIGN_ANY);
    EnableAutoHidePanes(CBRS_ALIGN_ANY);

    const DWORD style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_FLOAT_MULTI;

    dashboard_.Create(_T("Pulpit"), this, CRect(0, 0, 280, 240), TRUE, IDC_PANE_DASHBOARD,
                      style | CBRS_LEFT);
    dashboard_.EnableDocking(CBRS_ALIGN_ANY);
    DockPane(&dashboard_);

    movementLog_.Create(_T("Dziennik ruchów"), this, CRect(0, 0, 300, 220), TRUE,
                        IDC_PANE_MOVEMENTS, style | CBRS_RIGHT);
    movementLog_.EnableDocking(CBRS_ALIGN_ANY);
    DockPane(&movementLog_);
    movementLog_.List().InsertColumn(0, _T("Czas (UTC)"), LVCFMT_LEFT, 130);
    movementLog_.List().InsertColumn(1, _T("Ruch"), LVCFMT_LEFT, 44);
    movementLog_.List().InsertColumn(2, _T("SKU"), LVCFMT_LEFT, 54);
    movementLog_.List().InsertColumn(3, _T("Ilość"), LVCFMT_RIGHT, 48);

    details_.Create(_T("Szczegóły"), this, CRect(0, 0, 280, 160), TRUE, IDC_PANE_DETAILS,
                    style | CBRS_RIGHT);
    details_.EnableDocking(CBRS_ALIGN_ANY);
    // Tab Details together with the Movement log instead of stacking a second pane
    // on the right edge — two same-side panes clip each other when the window is
    // narrow; a tab group keeps both reachable at any width.
    CDockablePane* tabbed = nullptr;
    details_.AttachToTabWnd(&movementLog_, DM_SHOW, TRUE, &tabbed);
    details_.List().InsertColumn(0, _T("Pole"), LVCFMT_LEFT, 110);
    details_.List().InsertColumn(1, _T("Wartość"), LVCFMT_LEFT, 150);
}

void CMainFrame::RefreshPanes() {
    if (dashboard_.GetSafeHwnd() != nullptr) {
        dashboard_.Invalidate();
    }

    auto* doc = DYNAMIC_DOWNCAST(CWarehouseDoc, GetActiveDocument());
    if (doc != nullptr) {
        CString rows;
        rows.Format(_T("Indeksy: %d"), static_cast<int>(doc->Stock().size()));
        SetStatusPane(ID_INDICATOR_ROWS, rows);
    }

    CListCtrl& log = movementLog_.List();
    if (doc != nullptr && log.GetSafeHwnd() != nullptr) {
        log.DeleteAllItems();
        int i = 0;
        for (const warehouse::MovementRow& m : doc->Movements()) {
            log.InsertItem(i, FromUtf8(m.createdAt));
            log.SetItemText(i, 1, FromUtf8(m.type));
            log.SetItemText(i, 2, FromUtf8(m.sku));
            CString qty;
            qty.Format(_T("%d"), m.qty < 0 ? -m.qty : m.qty);
            log.SetItemText(i, 3, qty);
            ++i;
        }
    }
}

void CMainFrame::ShowDetails(const warehouse::StockRow& row) {
    CListCtrl& list = details_.List();
    if (list.GetSafeHwnd() == nullptr) {
        return;
    }
    list.DeleteAllItems();
    const auto add = [&list](const CString& field, const CString& value) {
        const int i = list.GetItemCount();
        list.InsertItem(i, field);
        list.SetItemText(i, 1, value);
    };
    CString onHand;
    onHand.Format(_T("%d"), row.onHand);
    add(_T("SKU"), FromUtf8(row.sku));
    add(_T("Produkt"), FromUtf8(row.productName));
    add(_T("Magazyn"), FromUtf8(row.warehouseCode + " " + row.warehouseName));
    add(_T("Stan na rękę"), onHand);
    add(_T("Niski stan"), row.isLow ? _T("TAK") : _T("nie"));

    SetStatusPane(ID_INDICATOR_SKU, _T("SKU: ") + FromUtf8(row.sku));
}

CDockablePane* CMainFrame::PaneFor(UINT cmdId) {
    switch (cmdId) {
        case ID_TOGGLE_DASHBOARD: return &dashboard_;
        case ID_TOGGLE_MOVEMENTS: return &movementLog_;
        case ID_TOGGLE_DETAILS:   return &details_;
        default:                  return nullptr;
    }
}

void CMainFrame::OnViewPane(UINT cmdId) {
    if (CDockablePane* pane = PaneFor(cmdId)) {
        pane->ShowPane(!pane->IsVisible(), FALSE, TRUE);
        RecalcLayout();
    }
}

void CMainFrame::OnUpdateViewPane(CCmdUI* cmdUI) {
    CDockablePane* pane = PaneFor(cmdUI->m_nID);
    cmdUI->SetCheck(pane != nullptr && pane->IsVisible());
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
