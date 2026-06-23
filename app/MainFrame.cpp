#include "framework.h"
#include "resource.h"

#include <afxvisualmanageroffice2003.h>
#include <afxvisualmanagervs2008.h>
#include <afxvisualmanagerwindows7.h>

#include "DarkVisualManager.h"
#include "MainFrame.h"
#include "StockView.h"
#include "TextUtil.h"
#include "WarehouseDoc.h"
#include "warehouse/stock_repository.hpp"

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
    ON_WM_CREATE()
    ON_COMMAND_RANGE(ID_THEME_OFFICE_BLUE, ID_THEME_DARK, &CMainFrame::OnTheme)
    ON_UPDATE_COMMAND_UI_RANGE(ID_THEME_OFFICE_BLUE, ID_THEME_DARK, &CMainFrame::OnUpdateTheme)
    ON_UPDATE_COMMAND_UI(ID_THEME_MENU, &CMainFrame::OnUpdateThemeMenu)
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
// Show just the product title (the IDR_MAINFRAME window title, now "Stany Magazynowe").
// Passing FALSE suppresses MFC's "{doc title} - " prefix; going through the framework
// method (not a raw SetWindowText) also refreshes the themed CFrameWndEx caption.
void CMainFrame::OnUpdateFrameTitle(BOOL /*bAddToTitle*/) {
    CFrameWndEx::OnUpdateFrameTitle(FALSE);
}

void CMainFrame::CreateStatusBar() {
    statusBar_.Create(this);
    statusBar_.AddElement(new CMFCRibbonStatusBarPane(ID_INDICATOR_ROWS, _T("Pozycje: 0000"), TRUE), _T(""));
    statusBar_.AddElement(new CMFCRibbonStatusBarPane(ID_INDICATOR_SKU, _T("Symbol: —"), TRUE), _T(""));
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
    auto* themeMenu = new CMFCRibbonButton(ID_THEME_MENU, _T("Motyw"), 0, 0);
    themeMenu->SetDefaultCommand(FALSE);  // whole button opens the menu (not a split button)
    themeMenu->AddSubItem(new CMFCRibbonButton(ID_THEME_OFFICE_BLUE, _T("Office 2007 — niebieski")));
    themeMenu->AddSubItem(new CMFCRibbonButton(ID_THEME_OFFICE_BLACK, _T("Office 2007 — czarny")));
    themeMenu->AddSubItem(new CMFCRibbonButton(ID_THEME_OFFICE_SILVER, _T("Office 2007 — srebrny")));
    themeMenu->AddSubItem(new CMFCRibbonButton(ID_THEME_OFFICE_AQUA, _T("Office 2007 — aqua")));
    themeMenu->AddSubItem(new CMFCRibbonButton(ID_THEME_OFFICE2003, _T("Office 2003")));
    themeMenu->AddSubItem(new CMFCRibbonButton(ID_THEME_VS2008, _T("Visual Studio 2008")));
    themeMenu->AddSubItem(new CMFCRibbonButton(ID_THEME_WINDOWS7, _T("Windows 7")));
    themeMenu->AddSubItem(new CMFCRibbonButton(ID_THEME_DARK, _T("Ciemny (pełny)")));
    motyw->Add(themeMenu);

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

    // Wide enough for all five auto-sized columns (incl. the full "YYYY-MM-DD HH:MM:SS"
    // timestamp and the vertical scrollbar) to show without a horizontal scrollbar.
    movementLog_.Create(_T("Dziennik ruchów"), this, CRect(0, 0, 384, 220), TRUE,
                        IDC_PANE_MOVEMENTS, style | CBRS_RIGHT);
    movementLog_.EnableDocking(CBRS_ALIGN_ANY);
    DockPane(&movementLog_);
    movementLog_.List().InsertColumn(0, _T("Czas"), LVCFMT_LEFT, 130);
    movementLog_.List().InsertColumn(1, _T("Magazyn"), LVCFMT_LEFT, 60);
    movementLog_.List().InsertColumn(2, _T("Ruch"), LVCFMT_LEFT, 44);
    movementLog_.List().InsertColumn(3, _T("Symbol"), LVCFMT_LEFT, 60);
    movementLog_.List().InsertColumn(4, _T("Ilość"), LVCFMT_RIGHT, 48);

    details_.Create(_T("Szczegóły"), this, CRect(0, 0, 280, 160), TRUE, IDC_PANE_DETAILS,
                    style | CBRS_RIGHT);
    details_.EnableDocking(CBRS_ALIGN_ANY);
    // Tab Details together with the Movement log instead of stacking a second pane
    // on the right edge — two same-side panes clip each other when the window is
    // narrow; a tab group keeps both reachable at any width.
    CDockablePane* tabbed = nullptr;
    details_.AttachToTabWnd(&movementLog_, DM_SHOW, TRUE, &tabbed);
}

void CMainFrame::RefreshPanes() {
    if (dashboard_.GetSafeHwnd() != nullptr) {
        dashboard_.Invalidate();
    }

    auto* doc = DYNAMIC_DOWNCAST(CWarehouseDoc, GetActiveDocument());
    if (doc != nullptr) {
        CString rows;
        rows.Format(_T("Pozycje: %d"), static_cast<int>(doc->Stock().size()));
        SetStatusPane(ID_INDICATOR_ROWS, rows);
    }

    CListCtrl& log = movementLog_.List();
    if (doc != nullptr && log.GetSafeHwnd() != nullptr) {
        movementLog_.SetRows(&doc->Movements());  // backs the column sort
        log.DeleteAllItems();
        int i = 0;
        for (const warehouse::MovementRow& m : doc->Movements()) {
            log.InsertItem(i, FromUtf8(m.createdAt));
            log.SetItemText(i, 1, FromUtf8(m.warehouseCode));
            log.SetItemText(i, 2, FromUtf8(m.type));
            log.SetItemText(i, 3, FromUtf8(m.sku));
            CString qty;
            qty.Format(_T("%d"), m.qty < 0 ? -m.qty : m.qty);
            log.SetItemText(i, 4, qty);
            log.SetItemData(i, static_cast<DWORD_PTR>(i));  // index into Movements()
            ++i;
        }
        movementLog_.Resort();  // keep the active sort (default: Czas descending) after reload
        // Fit every column to its widest value (and header) so the log is readable on first
        // sight — no dragging column borders to reveal the timestamp / values.
        constexpr int kMovementColumns = 5;
        for (int col = 0; col < kMovementColumns; ++col) {
            log.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
        }
    }
}

void CMainFrame::ShowDetails(const warehouse::StockRow& row) {
    details_.Show(row);
    SetStatusPane(ID_INDICATOR_SKU, _T("Symbol: ") + FromUtf8(row.sku));
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

// Apply one of the built-in MFC visual managers. The Office 2007 manager needs its
// colour style set before it becomes the default; the others are self-contained.
void CMainFrame::OnTheme(UINT cmdId) {
    using Office2007 = CMFCVisualManagerOffice2007;
    switch (cmdId) {
        case ID_THEME_OFFICE_BLUE:
            Office2007::SetStyle(Office2007::Office2007_LunaBlue);
            CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(Office2007));
            break;
        case ID_THEME_OFFICE_BLACK:
            Office2007::SetStyle(Office2007::Office2007_ObsidianBlack);
            CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(Office2007));
            break;
        case ID_THEME_OFFICE_SILVER:
            Office2007::SetStyle(Office2007::Office2007_Silver);
            CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(Office2007));
            break;
        case ID_THEME_OFFICE_AQUA:
            Office2007::SetStyle(Office2007::Office2007_Aqua);
            CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(Office2007));
            break;
        case ID_THEME_OFFICE2003:
            CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
            break;
        case ID_THEME_VS2008:
            CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2008));
            break;
        case ID_THEME_WINDOWS7:
            CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));
            break;
        case ID_THEME_DARK:
            Office2007::SetStyle(Office2007::Office2007_ObsidianBlack);  // dark chrome base
            CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CDarkVisualManager));
            break;
        default:
            return;
    }
    // The list/grid header draws its text in the global button-text colour (not a
    // visual-manager hook), so flip it light for the dark theme and back otherwise.
    const bool dark = (cmdId == ID_THEME_DARK);
    GetGlobalData()->clrBtnText = dark ? RGB(220, 220, 220) : ::GetSysColor(COLOR_BTNTEXT);
    ApplyContentTheme(dark);
    currentTheme_ = cmdId;
    RedrawWindow(nullptr, nullptr, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);
}

// Recolour the non-Feature-Pack content (grids, dock list, owner-drawn dashboard,
// property grid) for the dark theme; all light themes use the framework defaults.
void CMainFrame::ApplyContentTheme(bool dark) {
    dashboard_.SetDark(dark);
    movementLog_.SetDark(dark);
    details_.SetDark(dark);
    if (auto* view = DYNAMIC_DOWNCAST(CStockView, GetActiveView())) {
        view->SetDark(dark);
    }
}

void CMainFrame::OnUpdateTheme(CCmdUI* cmdUI) {
    cmdUI->SetRadio(cmdUI->m_nID == currentTheme_);  // dot the active theme
}

// The drop-down container has no command of its own; keep it enabled so its menu opens
// (MFC would otherwise auto-disable a command with no ON_COMMAND handler).
void CMainFrame::OnUpdateThemeMenu(CCmdUI* cmdUI) {
    cmdUI->Enable(TRUE);
}
