#include "framework.h"
#include "resource.h"

#include <shellapi.h>  // ShellExecute (restart on layout reset)

#include <afxvisualmanageroffice2003.h>
#include <afxvisualmanagervs2008.h>
#include <afxvisualmanagerwindows7.h>

#include "DarkVisualManager.h"
#include "I18n.h"
#include "MainFrame.h"
#include "StockView.h"
#include "WarehouseApp.h"
#include "TextUtil.h"
#include "WarehouseDoc.h"
#include "warehouse/stock_repository.hpp"

using i18n::T;

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
    ON_WM_CREATE()
    ON_COMMAND_RANGE(ID_THEME_OFFICE_BLUE, ID_THEME_DARK, &CMainFrame::OnTheme)
    ON_UPDATE_COMMAND_UI_RANGE(ID_THEME_OFFICE_BLUE, ID_THEME_DARK, &CMainFrame::OnUpdateTheme)
    ON_UPDATE_COMMAND_UI(ID_THEME_MENU, &CMainFrame::OnUpdateMenuButton)
    ON_COMMAND_RANGE(ID_TOGGLE_DASHBOARD, ID_TOGGLE_DETAILS, &CMainFrame::OnViewPane)
    ON_UPDATE_COMMAND_UI_RANGE(ID_TOGGLE_DASHBOARD, ID_TOGGLE_DETAILS, &CMainFrame::OnUpdateViewPane)
    ON_COMMAND_RANGE(ID_LANG_POLISH, ID_LANG_ENGLISH, &CMainFrame::OnLanguage)
    ON_UPDATE_COMMAND_UI_RANGE(ID_LANG_POLISH, ID_LANG_ENGLISH, &CMainFrame::OnUpdateLanguage)
    ON_UPDATE_COMMAND_UI(ID_LANG_MENU, &CMainFrame::OnUpdateMenuButton)
    ON_COMMAND(ID_VIEW_RESET_LAYOUT, &CMainFrame::OnResetLayout)
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
    SetTitle(T(i18n::AppTitle));  // localise the caption (m_strTitle defaults to the PL resource)
    CFrameWndEx::OnUpdateFrameTitle(FALSE);
}

void CMainFrame::CreateStatusBar() {
    statusBar_.Create(this);
    statusBar_.AddElement(new CMFCRibbonStatusBarPane(ID_INDICATOR_ROWS, T(i18n::StRowsSizing), TRUE), _T(""));
    statusBar_.AddElement(new CMFCRibbonStatusBarPane(ID_INDICATOR_SKU, T(i18n::StSkuEmpty), TRUE), _T(""));
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

    // Round application button (top-left of the caption) carrying the app icon — the
    // Office-style way to show the app's icon in a ribbon title bar. Its menu holds Exit.
    ribbon_.SetApplicationButton(new CMFCRibbonApplicationButton(IDB_APPBTN), CSize(45, 45));
    CMFCRibbonMainPanel* mainPanel =
        ribbon_.AddMainCategory(T(i18n::AppTitle), IDB_RIBBON_MAGAZYN_16, IDB_RIBBON_MAGAZYN_32);
    mainPanel->Add(new CMFCRibbonButton(ID_APP_EXIT, T(i18n::MenuExit), -1, -1));

    // Each category owns a small (16px) + large (32px) image strip; the button's
    // image index selects its glyph within that category's strip.
    CMFCRibbonCategory* magazyn =
        ribbon_.AddCategory(T(i18n::TabStock), IDB_RIBBON_MAGAZYN_16, IDB_RIBBON_MAGAZYN_32);

    CMFCRibbonPanel* stany = magazyn->AddPanel(T(i18n::PanelStock));
    stany->Add(new CMFCRibbonButton(ID_STOCK_REFRESH, T(i18n::BtnRefresh), 0, 0));
    stany->Add(new CMFCRibbonButton(ID_STOCK_FILTER_LOW, T(i18n::BtnLowOnly), 1, 1));

    CMFCRibbonPanel* ruchy = magazyn->AddPanel(T(i18n::PanelMovements));
    ruchy->Add(new CMFCRibbonButton(ID_STOCK_RECORD_IN, T(i18n::BtnReceive), 2, 2));
    ruchy->Add(new CMFCRibbonButton(ID_STOCK_RECORD_OUT, T(i18n::BtnIssue), 3, 3));

    CMFCRibbonPanel* edycja = magazyn->AddPanel(T(i18n::PanelEdit));
    edycja->Add(new CMFCRibbonButton(ID_EDIT_UNDO, T(i18n::BtnUndo), 4, 4));
    edycja->Add(new CMFCRibbonButton(ID_EDIT_REDO, T(i18n::BtnRedo), 5, 5));

    CMFCRibbonCategory* widok =
        ribbon_.AddCategory(T(i18n::TabView), IDB_RIBBON_WIDOK_16, IDB_RIBBON_WIDOK_32);
    CMFCRibbonPanel* motyw = widok->AddPanel(T(i18n::PanelTheme));
    auto* themeMenu = new CMFCRibbonButton(ID_THEME_MENU, T(i18n::BtnTheme), 0, 0);
    themeMenu->SetDefaultCommand(FALSE);  // whole button opens the menu (not a split button)
    themeMenu->AddSubItem(new CMFCRibbonButton(ID_THEME_OFFICE_BLUE, T(i18n::ThemeOfficeBlue)));
    themeMenu->AddSubItem(new CMFCRibbonButton(ID_THEME_OFFICE_BLACK, T(i18n::ThemeOfficeBlack)));
    themeMenu->AddSubItem(new CMFCRibbonButton(ID_THEME_OFFICE_SILVER, T(i18n::ThemeOfficeSilver)));
    themeMenu->AddSubItem(new CMFCRibbonButton(ID_THEME_OFFICE_AQUA, T(i18n::ThemeOfficeAqua)));
    themeMenu->AddSubItem(new CMFCRibbonButton(ID_THEME_OFFICE2003, T(i18n::ThemeOffice2003)));
    themeMenu->AddSubItem(new CMFCRibbonButton(ID_THEME_VS2008, T(i18n::ThemeVS2008)));
    themeMenu->AddSubItem(new CMFCRibbonButton(ID_THEME_WINDOWS7, T(i18n::ThemeWindows7)));
    themeMenu->AddSubItem(new CMFCRibbonButton(ID_THEME_DARK, T(i18n::ThemeDark)));
    motyw->Add(themeMenu);

    CMFCRibbonPanel* panele = widok->AddPanel(T(i18n::PanelPanes));
    panele->Add(new CMFCRibbonButton(ID_TOGGLE_DASHBOARD, T(i18n::BtnDashboard), 1, 1));
    panele->Add(new CMFCRibbonButton(ID_TOGGLE_MOVEMENTS, T(i18n::BtnJournal), 2, 2));
    panele->Add(new CMFCRibbonButton(ID_TOGGLE_DETAILS, T(i18n::BtnDetails), 3, 3));
    panele->Add(new CMFCRibbonButton(ID_VIEW_RESET_LAYOUT, T(i18n::BtnResetLayout), 5, 5));  // window-layout glyph

    CMFCRibbonPanel* jezyk = widok->AddPanel(T(i18n::PanelLanguage));
    // Image index 4 = the globe glyph appended to the Widok strip (slots 0-3 are the existing icons).
    auto* langMenu = new CMFCRibbonButton(ID_LANG_MENU, T(i18n::BtnLanguage), 4, 4);
    langMenu->SetDefaultCommand(FALSE);
    langMenu->AddSubItem(new CMFCRibbonButton(ID_LANG_POLISH, T(i18n::LangPolish)));
    langMenu->AddSubItem(new CMFCRibbonButton(ID_LANG_ENGLISH, T(i18n::LangEnglish)));
    jezyk->Add(langMenu);
}

// Three docking panels managed by CFrameWndEx: a (custom-drawn) Dashboard on the
// left, and the Movement-log + Details list panes on the right.
void CMainFrame::CreatePanes() {
    CDockingManager::SetDockingMode(DT_SMART);
    EnableDocking(CBRS_ALIGN_ANY);
    EnableAutoHidePanes(CBRS_ALIGN_ANY);

    const DWORD style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_FLOAT_MULTI;

    dashboard_.Create(T(i18n::PaneDashboard), this, CRect(0, 0, 280, 240), TRUE, IDC_PANE_DASHBOARD,
                      style | CBRS_LEFT);
    dashboard_.EnableDocking(CBRS_ALIGN_ANY);
    DockPane(&dashboard_);

    // Wide enough for all five auto-sized columns (incl. the full "YYYY-MM-DD HH:MM:SS"
    // timestamp and the vertical scrollbar) to show without a horizontal scrollbar.
    movementLog_.Create(T(i18n::PaneMovements), this, CRect(0, 0, 384, 220), TRUE,
                        IDC_PANE_MOVEMENTS, style | CBRS_RIGHT);
    movementLog_.EnableDocking(CBRS_ALIGN_ANY);
    DockPane(&movementLog_);
    movementLog_.List().InsertColumn(0, T(i18n::ColTime), LVCFMT_LEFT, 130);
    movementLog_.List().InsertColumn(1, T(i18n::ColWarehouse), LVCFMT_LEFT, 60);
    movementLog_.List().InsertColumn(2, T(i18n::ColMoveType), LVCFMT_LEFT, 44);
    movementLog_.List().InsertColumn(3, T(i18n::ColSku), LVCFMT_LEFT, 60);
    movementLog_.List().InsertColumn(4, T(i18n::ColQty), LVCFMT_RIGHT, 48);

    details_.Create(T(i18n::PaneDetails), this, CRect(0, 0, 280, 160), TRUE, IDC_PANE_DETAILS,
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

// Row count in the status bar. When the grid is filtered, show "shown z all" so the count
// isn't read as inconsistent with the visibly shorter grid; otherwise just the total.
void CMainFrame::SetRowCount(int visible, int total, bool filtered) {
    CString text;
    if (filtered) {
        text.Format(T(i18n::StRowsFilteredFmt), visible, total);
    } else {
        text.Format(T(i18n::StRowsFmt), total);
    }
    SetStatusPane(ID_INDICATOR_ROWS, text);
}

void CMainFrame::ShowDetails(const warehouse::StockRow& row) {
    details_.Show(row);
    SetStatusPane(ID_INDICATOR_SKU, CString(T(i18n::StSkuPrefix)) + FromUtf8(row.sku));
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

// The whole UI is built in the chosen language at startup, so a switch only records the
// choice and asks the user to restart (no live re-translation of the ribbon/panes).
void CMainFrame::OnLanguage(UINT cmdId) {
    const i18n::Lang lang =
        (cmdId == ID_LANG_ENGLISH) ? i18n::Lang::English : i18n::Lang::Polish;
    if (lang == i18n::Current()) {
        return;
    }
    i18n::Persist(lang);
    MessageBox(T(i18n::MsgRestart), T(i18n::AppTitle), MB_ICONINFORMATION | MB_OK);
}

void CMainFrame::OnUpdateLanguage(CCmdUI* cmdUI) {
    const i18n::Lang lang =
        (cmdUI->m_nID == ID_LANG_ENGLISH) ? i18n::Lang::English : i18n::Lang::Polish;
    cmdUI->SetCheck(lang == i18n::Current() ? 1 : 0);
}

// Restore the default pane layout. The Feature Pack persists docking/window state to the
// registry, so a user can strand themselves (a floated/hidden/off-screen pane that survives
// restarts). This drops the saved state and relaunches into the built-in default layout.
void CMainFrame::OnResetLayout() {
    if (MessageBox(T(i18n::MsgResetLayout), T(i18n::AppTitle), MB_ICONQUESTION | MB_YESNO) != IDYES) {
        return;
    }
    auto* app = static_cast<CWarehouseApp*>(AfxGetApp());
    app->SkipStateSaveOnExit();  // don't write the current (confusing) layout back on exit
    CString workspace;
    workspace.Format(_T("Software\\%s\\%s\\Workspace"), app->m_pszRegistryKey, app->m_pszProfileName);
    ::RegDeleteTree(HKEY_CURRENT_USER, workspace);  // remove the previously saved layout too

    TCHAR exe[MAX_PATH] = {};
    ::GetModuleFileName(nullptr, exe, MAX_PATH);
    ::ShellExecute(nullptr, nullptr, exe, nullptr, nullptr, SW_SHOWNORMAL);  // launch a fresh instance
    PostMessage(WM_CLOSE);
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

// A drop-down container button (Motyw, Język) has no command of its own; keep it enabled so
// its menu opens (MFC would otherwise auto-disable a command with no ON_COMMAND handler).
void CMainFrame::OnUpdateMenuButton(CCmdUI* cmdUI) {
    cmdUI->Enable(TRUE);
}
