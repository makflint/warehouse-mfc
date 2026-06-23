#pragma once

#include "framework.h"

// Lightweight UI localisation. The app's chrome and runtime messages are built in C++, so a
// single string catalog (keyed by Tx) with a Polish/English column is enough. The language is
// chosen once at startup (registry override, else the Windows UI language), so all UI is built
// in that language and switching just needs a restart — no live re-translation.
//
// Out of scope (kept Polish): database content (product/warehouse names) and the SQL Server
// stored-proc error text; those are real-world data, not UI strings.

namespace i18n {

enum class Lang { Polish = 0, English = 1 };

// String keys. Keep in sync with the table in I18n.cpp.
enum Tx {
    AppTitle,

    // Ribbon — categories / panels
    TabStock, TabView,
    PanelStock, PanelMovements, PanelEdit, PanelTheme, PanelPanes, PanelLanguage,
    // Ribbon — buttons
    BtnRefresh, BtnLowOnly, BtnReceive, BtnIssue, BtnUndo, BtnRedo,
    BtnTheme, BtnDashboard, BtnJournal, BtnDetails, BtnResetLayout, BtnLanguage,
    LangPolish, LangEnglish,
    // Theme menu items
    ThemeOfficeBlue, ThemeOfficeBlack, ThemeOfficeSilver, ThemeOfficeAqua,
    ThemeOffice2003, ThemeVS2008, ThemeWindows7, ThemeDark,

    // Dock panes
    PaneDashboard, PaneMovements, PaneDetails,

    // Main stock grid columns
    ColWarehouse, ColSku, ColProduct, ColOnHand,
    // Movement-log columns
    ColTime, ColMoveType, ColQty,

    // Dashboard
    KpiAssortment, KpiLowStock, KpiTotalUnits, ChartTitle,

    // Details pane
    HdrField, HdrValue, DtOnHand, DtLow, YesValue, NoValue,

    // Record dialog
    DlgReceiveTitle, DlgIssueTitle, LblProduct, LblWarehouse, LblQty, BtnOK, BtnCancel,
    MsgSelectBoth, MsgQtyRangeFmt,

    // Status bar
    StRowsFmt, StRowsFilteredFmt, StRowsSizing, StSkuPrefix, StSkuEmpty,

    // Messages
    MsgRestart, MsgResetLayout,

    // Application-button menu
    MenuExit,

    // Help / About
    HelpButton, HelpAbout, AboutDesc, AboutVersion, AboutShortcuts,

    Count
};

// Active language; defaults to Polish until Init() runs.
Lang Current();

// Resolve the startup language (registry override, else GetUserDefaultUILanguage) and make it
// current. Call once in InitInstance, after SetRegistryKey.
void Init();

// Persist a chosen language for the next launch (does not change the running UI).
void Persist(Lang lang);

// Localised string for the active language.
LPCTSTR T(Tx id);

}  // namespace i18n
