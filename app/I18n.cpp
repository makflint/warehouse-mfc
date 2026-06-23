#include "framework.h"

#include "I18n.h"

namespace i18n {
namespace {

Lang g_lang = Lang::Polish;

// [Tx][Polish, English]. Order must match enum Tx exactly.
const LPCTSTR kCatalog[Tx::Count][2] = {
    /* AppTitle        */ {_T("Stany Magazynowe"), _T("Warehouse Stock")},

    /* TabStock        */ {_T("Magazyn"), _T("Stock")},
    /* TabView         */ {_T("Widok"), _T("View")},
    /* PanelStock      */ {_T("Stany"), _T("Stock")},
    /* PanelMovements  */ {_T("Ruchy"), _T("Movements")},
    /* PanelEdit       */ {_T("Edycja"), _T("Edit")},
    /* PanelTheme      */ {_T("Motyw"), _T("Theme")},
    /* PanelPanes      */ {_T("Panele"), _T("Panes")},
    /* PanelLanguage   */ {_T("Język"), _T("Language")},

    /* BtnRefresh      */ {_T("Odśwież"), _T("Refresh")},
    /* BtnLowOnly      */ {_T("Tylko niskie"), _T("Low only")},
    /* BtnReceive      */ {_T("Przyjmij"), _T("Receive")},
    /* BtnIssue        */ {_T("Wydaj"), _T("Issue")},
    /* BtnUndo         */ {_T("Cofnij"), _T("Undo")},
    /* BtnRedo         */ {_T("Ponów"), _T("Redo")},
    /* BtnTheme        */ {_T("Motyw"), _T("Theme")},
    /* BtnDashboard    */ {_T("Pulpit"), _T("Dashboard")},
    /* BtnJournal      */ {_T("Dziennik"), _T("Journal")},
    /* BtnDetails      */ {_T("Szczegóły"), _T("Details")},
    /* BtnResetLayout  */ {_T("Przywróć układ okien"), _T("Restore layout")},
    /* BtnLanguage     */ {_T("Język"), _T("Language")},
    /* LangPolish      */ {_T("Polski"), _T("Polish")},
    /* LangEnglish     */ {_T("Angielski"), _T("English")},

    /* ThemeOfficeBlue   */ {_T("Office 2007 — niebieski"), _T("Office 2007 — Blue")},
    /* ThemeOfficeBlack  */ {_T("Office 2007 — czarny"), _T("Office 2007 — Black")},
    /* ThemeOfficeSilver */ {_T("Office 2007 — srebrny"), _T("Office 2007 — Silver")},
    /* ThemeOfficeAqua   */ {_T("Office 2007 — aqua"), _T("Office 2007 — Aqua")},
    /* ThemeOffice2003   */ {_T("Office 2003"), _T("Office 2003")},
    /* ThemeVS2008       */ {_T("Visual Studio 2008"), _T("Visual Studio 2008")},
    /* ThemeWindows7     */ {_T("Windows 7"), _T("Windows 7")},
    /* ThemeDark         */ {_T("Ciemny (pełny)"), _T("Dark (full)")},

    /* PaneDashboard   */ {_T("Pulpit"), _T("Dashboard")},
    /* PaneMovements   */ {_T("Dziennik ruchów"), _T("Movement journal")},
    /* PaneDetails     */ {_T("Szczegóły"), _T("Details")},

    /* ColWarehouse    */ {_T("Magazyn"), _T("Warehouse")},
    /* ColSku          */ {_T("Symbol"), _T("SKU")},
    /* ColProduct      */ {_T("Produkt"), _T("Product")},
    /* ColOnHand       */ {_T("Stan"), _T("On hand")},
    /* ColTime         */ {_T("Czas"), _T("Time")},
    /* ColMoveType     */ {_T("Ruch"), _T("Type")},
    /* ColQty          */ {_T("Ilość"), _T("Qty")},

    /* KpiAssortment   */ {_T("Asortyment"), _T("Assortment")},
    /* KpiLowStock     */ {_T("Niskie stany magazynowe"), _T("Low stock")},
    /* KpiTotalUnits   */ {_T("Suma sztuk"), _T("Total units")},
    /* ChartTitle      */ {_T("Stan magazynowy wg indeksu"), _T("On-hand by SKU")},

    /* HdrField        */ {_T("Pole"), _T("Field")},
    /* HdrValue        */ {_T("Wartość"), _T("Value")},
    /* DtOnHand        */ {_T("Stan magazynowy"), _T("On hand")},
    /* DtLow           */ {_T("Niski stan"), _T("Low stock")},
    /* YesValue        */ {_T("TAK"), _T("YES")},
    /* NoValue         */ {_T("nie"), _T("no")},

    /* DlgReceiveTitle */ {_T("Przyjęcie (IN)"), _T("Receipt (IN)")},
    /* DlgIssueTitle   */ {_T("Wydanie (OUT)"), _T("Issue (OUT)")},
    /* LblProduct      */ {_T("Produkt:"), _T("Product:")},
    /* LblWarehouse    */ {_T("Magazyn:"), _T("Warehouse:")},
    /* LblQty          */ {_T("Ilość:"), _T("Quantity:")},
    /* BtnOK           */ {_T("OK"), _T("OK")},
    /* BtnCancel       */ {_T("Anuluj"), _T("Cancel")},
    /* MsgSelectBoth   */ {_T("Wybierz produkt i magazyn."), _T("Select a product and a warehouse.")},
    /* MsgQtyRangeFmt  */ {_T("Podaj ilość z zakresu od %d do %d."), _T("Enter a quantity between %d and %d.")},

    /* StRowsFmt        */ {_T("Pozycje: %d"), _T("Items: %d")},
    /* StRowsFilteredFmt*/ {_T("Pozycje: %d z %d"), _T("Items: %d of %d")},
    /* StRowsSizing     */ {_T("Pozycje: 000 z 000"), _T("Items: 000 of 000")},
    /* StSkuPrefix      */ {_T("Symbol: "), _T("SKU: ")},
    /* StSkuEmpty       */ {_T("Symbol: —"), _T("SKU: —")},

    /* MsgRestart       */ {_T("Zmiana języka zostanie zastosowana po ponownym uruchomieniu aplikacji."),
                            _T("The language change will take effect after you restart the app.")},
    /* MsgResetLayout   */ {_T("Przywrócić domyślny układ paneli? Aplikacja zostanie uruchomiona ponownie."),
                            _T("Restore the default pane layout? The app will restart.")},
    /* MenuExit         */ {_T("Zakończ"), _T("Exit")},
};

}  // namespace

Lang Current() { return g_lang; }

void Init() {
    const int saved = AfxGetApp()->GetProfileInt(_T("Settings"), _T("Language"), -1);
    if (saved == static_cast<int>(Lang::Polish) || saved == static_cast<int>(Lang::English)) {
        g_lang = static_cast<Lang>(saved);
        return;
    }
    // No saved preference: follow the Windows UI language (Polish only for a Polish system).
    g_lang = (PRIMARYLANGID(::GetUserDefaultUILanguage()) == LANG_POLISH) ? Lang::Polish
                                                                          : Lang::English;
}

void Persist(Lang lang) {
    AfxGetApp()->WriteProfileInt(_T("Settings"), _T("Language"), static_cast<int>(lang));
}

LPCTSTR T(Tx id) { return kCatalog[id][static_cast<int>(g_lang)]; }

}  // namespace i18n
