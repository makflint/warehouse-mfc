#pragma once

#define IDR_MAINFRAME       128
#define IDD_RECORD_MOVEMENT 130
#define IDD_ABOUTBOX        131

// About-dialog static labels (set at runtime for the active language).
#define IDC_ABOUT_TITLE 1020
#define IDC_ABOUT_DESC  1021
#define IDC_ABOUT_VER   1022
#define IDC_ABOUT_KEYS  1023

#define IDC_PRODUCT   1001
#define IDC_WAREHOUSE 1002
#define IDC_QTY       1003
// Unique ids for the dialog's static labels, so each can be localised at runtime.
#define IDC_LBL_PRODUCT   1010
#define IDC_LBL_WAREHOUSE 1011
#define IDC_LBL_QTY       1012

#define ID_STOCK_REFRESH    32771
#define ID_STOCK_RECORD_IN  32772
#define ID_STOCK_RECORD_OUT 32773
#define ID_STOCK_FILTER_LOW 32774

// Theme picker (Widok tab). The applied-theme ids must stay contiguous for the
// ON_COMMAND_RANGE handler; ID_THEME_MENU is just the drop-down container.
#define ID_THEME_OFFICE_BLUE   32790
#define ID_THEME_OFFICE_BLACK  32791
#define ID_THEME_OFFICE_SILVER 32792
#define ID_THEME_OFFICE_AQUA   32793
#define ID_THEME_OFFICE2003    32794
#define ID_THEME_VS2008        32795
#define ID_THEME_WINDOWS7      32796
#define ID_THEME_DARK          32797
#define ID_THEME_MENU          32798

// Dockable panes (control ids) and their Widok-tab show/hide toggles.
#define IDC_PANE_DASHBOARD  1201
#define IDC_PANE_MOVEMENTS  1202
#define IDC_PANE_DETAILS    1203
#define ID_TOGGLE_DASHBOARD 32776
#define ID_TOGGLE_MOVEMENTS 32777
#define ID_TOGGLE_DETAILS   32778

// Language picker (Widok tab). Applied-language ids stay contiguous for ON_COMMAND_RANGE;
// ID_LANG_MENU is the drop-down container.
#define ID_LANG_POLISH  32780
#define ID_LANG_ENGLISH 32781
#define ID_LANG_MENU    32782

// Restore the default docking layout (clears saved state, restarts).
#define ID_VIEW_RESET_LAYOUT 32784

// Ribbon icon strips (16 px small + 32 px large), one per ribbon category.
#define IDB_RIBBON_MAGAZYN_16 200
#define IDB_RIBBON_MAGAZYN_32 201
#define IDB_RIBBON_WIDOK_16   202
#define IDB_RIBBON_WIDOK_32   203
#define IDB_APPBTN            204  // ribbon application-button (round orb) image

// Status-bar indicator panes (also their string-table ids, which size each pane).
#define ID_INDICATOR_ROWS 0xE720
#define ID_INDICATOR_SKU  0xE721
#define ID_INDICATOR_DB   0xE722
