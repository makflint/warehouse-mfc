#include "framework.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "DockPanes.h"
#include "I18n.h"
#include "TextUtil.h"
#include "WarehouseDoc.h"
#include "warehouse/view_logic.hpp"

using i18n::T;

namespace {

// The stock snapshot the dashboard draws, taken from the active document.
const std::vector<warehouse::StockRow>* activeStock() {
    if (CFrameWnd* frame = DYNAMIC_DOWNCAST(CFrameWnd, AfxGetMainWnd())) {
        if (auto* doc = DYNAMIC_DOWNCAST(CWarehouseDoc, frame->GetActiveDocument())) {
            return &doc->Stock();
        }
    }
    return nullptr;
}

void drawTile(CDC& dc, CRect r, COLORREF fill, const CString& value, const CString& label,
              CFont& bigFont, CFont& smallFont) {
    dc.FillSolidRect(r, fill);
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(RGB(255, 255, 255));

    CRect top = r;
    top.bottom = r.top + r.Height() * 3 / 5;
    CFont* old = dc.SelectObject(&bigFont);
    dc.DrawText(value, top, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    CRect bottom = r;
    bottom.top = top.bottom;
    bottom.DeflateRect(3, 0);
    dc.SelectObject(&smallFont);
    // Word-break so multi-word labels (e.g. "Niskie stany magazynowe") wrap
    // instead of clipping inside the narrow tile.
    dc.DrawText(label, bottom, DT_CENTER | DT_TOP | DT_WORDBREAK);
    dc.SelectObject(old);
}

// The dashboard is owner-drawn, so the visual manager can't theme it — name both palettes here.
struct DashboardColors {
    COLORREF background, foreground, axis, bar, barLow;
};
DashboardColors dashboardColors(bool dark) {
    if (dark) {
        return {RGB(30, 30, 30), RGB(215, 215, 215), RGB(90, 90, 90), RGB(70, 120, 200), RGB(220, 80, 80)};
    }
    return {RGB(245, 246, 248), RGB(60, 60, 60), RGB(180, 180, 180), RGB(40, 86, 150), RGB(176, 32, 32)};
}

// KPI tile fills carry their own contrast, so they're theme-independent.
constexpr COLORREF kAssortmentFill = RGB(33, 115, 70);
constexpr COLORREF kLowStockFill = RGB(176, 32, 32);
constexpr COLORREF kLowStockEmptyFill = RGB(120, 120, 120);  // grey when nothing is low
constexpr COLORREF kTotalUnitsFill = RGB(40, 86, 150);

// Layout metrics (client-pixel units), all measured from the deflated inner rect.
constexpr int kMargin = 10;
constexpr int kTileHeight = 74;
constexpr int kTileGap = 8;
constexpr int kTilesToChartGap = 16;
constexpr int kChartTitleHeight = 16;
constexpr int kChartTitleToPlotGap = 20;
constexpr int kSkuLabelBand = 18;     // strip under the axis for SKU labels
constexpr int kBarValueHeight = 16;   // value label above each bar
constexpr int kMaxBarWidth = 48;
constexpr int kBigFontHeight = 30;    // KPI tile value
constexpr int kSmallFontHeight = 14;  // labels, chart text

void createDashboardFonts(CFont& bigFont, CFont& smallFont) {
    bigFont.CreateFont(kBigFontHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, _T("Segoe UI"));
    smallFont.CreateFont(kSmallFontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                         OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, _T("Segoe UI"));
}

// The figures behind the tiles + chart, aggregated per SKU across warehouses.
struct StockKpis {
    std::map<std::string, long long> onHandBySku;
    std::map<std::string, bool> lowBySku;
    long long totalUnits = 0;
    int lowRows = 0;
};
StockKpis computeKpis(const std::vector<warehouse::StockRow>& stock) {
    StockKpis kpis;
    for (const warehouse::StockRow& row : stock) {
        kpis.onHandBySku[row.sku] += row.onHand;
        kpis.lowBySku[row.sku] = kpis.lowBySku[row.sku] || row.isLow;
        kpis.totalUnits += row.onHand;
        if (row.isLow) {
            ++kpis.lowRows;
        }
    }
    return kpis;
}

// Three KPI tiles across the top of the inner rect.
void drawKpiTiles(CDC& dc, CRect inner, const StockKpis& kpis, CFont& bigFont, CFont& smallFont) {
    const int tileW = (inner.Width() - 2 * kTileGap) / 3;
    CString value;

    value.Format(_T("%d"), static_cast<int>(kpis.onHandBySku.size()));
    drawTile(dc, CRect(inner.left, inner.top, inner.left + tileW, inner.top + kTileHeight),
             kAssortmentFill, value, T(i18n::KpiAssortment), bigFont, smallFont);

    value.Format(_T("%d"), kpis.lowRows);
    drawTile(dc, CRect(inner.left + tileW + kTileGap, inner.top, inner.left + 2 * tileW + kTileGap, inner.top + kTileHeight),
             kpis.lowRows > 0 ? kLowStockFill : kLowStockEmptyFill, value, T(i18n::KpiLowStock), bigFont, smallFont);

    value.Format(_T("%lld"), kpis.totalUnits);
    drawTile(dc, CRect(inner.left + 2 * tileW + 2 * kTileGap, inner.top, inner.right, inner.top + kTileHeight),
             kTotalUnitsFill, value, T(i18n::KpiTotalUnits), bigFont, smallFont);
}

// Bar chart of on-hand per SKU, below the tiles. Low-stock SKUs draw in the low-stock colour.
void drawBarChart(CDC& dc, CRect inner, const StockKpis& kpis, const DashboardColors& colors, CFont& smallFont) {
    CRect chart = inner;
    chart.top += kTileHeight + kTilesToChartGap;
    dc.SetBkMode(TRANSPARENT);
    dc.SelectObject(&smallFont);
    dc.SetTextColor(colors.foreground);
    dc.DrawText(T(i18n::ChartTitle), CRect(chart.left, chart.top, chart.right, chart.top + kChartTitleHeight),
                DT_LEFT | DT_SINGLELINE);
    chart.top += kChartTitleToPlotGap;

    const int n = static_cast<int>(kpis.onHandBySku.size());
    if (n == 0) {
        return;
    }
    long long maxVal = 1;
    for (const auto& kv : kpis.onHandBySku) {
        maxVal = (std::max)(maxVal, kv.second);
    }

    const int axisY = chart.bottom - kSkuLabelBand;
    const int plotH = axisY - chart.top;
    const int slot = chart.Width() / n;
    const int barW = (std::min)(kMaxBarWidth, slot * 2 / 3);
    int i = 0;
    for (const auto& kv : kpis.onHandBySku) {
        const int cx = chart.left + slot * i + slot / 2;
        const int h = static_cast<int>(static_cast<long long>(plotH) * kv.second / maxVal);
        CRect bar(cx - barW / 2, axisY - h, cx + barW / 2, axisY);
        dc.FillSolidRect(bar, kpis.lowBySku.at(kv.first) ? colors.barLow : colors.bar);

        CString value;
        value.Format(_T("%lld"), kv.second);
        dc.SetTextColor(colors.foreground);
        dc.DrawText(value, CRect(cx - slot / 2, bar.top - kBarValueHeight, cx + slot / 2, bar.top),
                    DT_CENTER | DT_BOTTOM | DT_SINGLELINE);
        dc.DrawText(FromUtf8(kv.first), CRect(cx - slot / 2, axisY + 2, cx + slot / 2, axisY + kSkuLabelBand),
                    DT_CENTER | DT_TOP | DT_SINGLELINE);
        ++i;
    }
    dc.FillSolidRect(chart.left, axisY, chart.Width(), 1, colors.axis);
}

}  // namespace

// --- Dashboard pane --------------------------------------------------------
BEGIN_MESSAGE_MAP(CDashboardPane, CDockablePane)
    ON_WM_PAINT()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

// Tiles + chart are laid out from the client rect, so a resize (e.g. dragging the
// dock divider) must repaint the whole pane, not just the newly exposed strip.
void CDashboardPane::OnSize(UINT type, int cx, int cy) {
    CDockablePane::OnSize(type, cx, cy);
    Invalidate(FALSE);
}

// OnPaint repaints the entire client via a back-buffer, so suppress the default
// erase to avoid a flicker on resize.
BOOL CDashboardPane::OnEraseBkgnd(CDC* /*dc*/) {
    return TRUE;
}

void CDashboardPane::OnPaint() {
    CPaintDC paint(this);
    CRect rc;
    GetClientRect(rc);

    // Double-buffer to avoid flicker: draw everything to a memory DC, then blit once.
    CDC mem;
    mem.CreateCompatibleDC(&paint);
    CBitmap bmp;
    bmp.CreateCompatibleBitmap(&paint, rc.Width(), rc.Height());
    CBitmap* oldBmp = mem.SelectObject(&bmp);

    const DashboardColors colors = dashboardColors(dark_);
    mem.FillSolidRect(rc, colors.background);

    CFont bigFont, smallFont;
    createDashboardFonts(bigFont, smallFont);

    StockKpis kpis;
    if (const std::vector<warehouse::StockRow>* stock = activeStock()) {
        kpis = computeKpis(*stock);
    }

    CRect inner = rc;
    inner.DeflateRect(kMargin, kMargin);
    drawKpiTiles(mem, inner, kpis, bigFont, smallFont);
    drawBarChart(mem, inner, kpis, colors, smallFont);

    paint.BitBlt(0, 0, rc.Width(), rc.Height(), &mem, 0, 0, SRCCOPY);
    mem.SelectObject(oldBmp);
}

// --- Movement-log list (every column sorts) --------------------------------
// Items carry their Movements() index as item data, so each column compares the
// underlying field (numeric for Ilość, text otherwise) rather than the cell strings.
int CMovementLogList::OnCompareItems(LPARAM lParam1, LPARAM lParam2, int column) {
    if (rows_ == nullptr) {
        return 0;
    }
    const std::size_t a = static_cast<std::size_t>(lParam1);
    const std::size_t b = static_cast<std::size_t>(lParam2);
    if (a >= rows_->size() || b >= rows_->size()) {
        return 0;
    }
    const warehouse::MovementRow& ra = (*rows_)[a];
    const warehouse::MovementRow& rb = (*rows_)[b];
    return warehouse::compareMovement(ra, rb, static_cast<warehouse::MovementColumn>(column));
}

// Capture the active sort column/direction so Resort() can re-apply it after a reload.
void CMovementLogList::Sort(int column, BOOL ascending, BOOL add) {
    sortColumn_ = column;
    ascending_ = ascending;
    CMFCListCtrl::Sort(column, ascending, add);
}

void CMovementLogList::SetDark(bool dark) {
    dark_ = dark;
    if (GetSafeHwnd() == nullptr) {
        return;
    }
    const COLORREF bg = dark ? ThemeColorsFor(true).background : ::GetSysColor(COLOR_WINDOW);
    SetBkColor(bg);
    SetTextBkColor(bg);
    const DWORD ex = GetExtendedStyle();
    SetExtendedStyle(dark ? (ex & ~LVS_EX_GRIDLINES) : (ex | LVS_EX_GRIDLINES));
    EnableMarkSortedColumn(!dark, FALSE);  // light sort-column mark clashes on dark
    Invalidate();
}

COLORREF CMovementLogList::OnGetCellTextColor(int row, int column) {
    return dark_ ? ThemeColorsFor(true).text : CMFCListCtrl::OnGetCellTextColor(row, column);
}

COLORREF CMovementLogList::OnGetCellBkColor(int row, int column) {
    return dark_ ? ThemeColorsFor(true).background : CMFCListCtrl::OnGetCellBkColor(row, column);
}

// --- List pane (Movement log) ----------------------------------------------
BEGIN_MESSAGE_MAP(CListPane, CDockablePane)
    ON_WM_CREATE()
    ON_WM_SIZE()
END_MESSAGE_MAP()

int CListPane::OnCreate(LPCREATESTRUCT createStruct) {
    if (CDockablePane::OnCreate(createStruct) == -1) {
        return -1;
    }
    const DWORD style = WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS;
    list_.Create(style, CRect(0, 0, 0, 0), this, 1);
    list_.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    CreateUiFont(uiFont_);  // match the main grid + the rest of the modern UI
    if (uiFont_.GetSafeHandle() != nullptr) {
        list_.SetFont(&uiFont_);
    }
    return 0;
}

void CListPane::OnSize(UINT type, int cx, int cy) {
    CDockablePane::OnSize(type, cx, cy);
    if (list_.GetSafeHwnd() != nullptr) {
        list_.SetWindowPos(nullptr, 0, 0, cx, cy, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

// --- Details pane (Feature-Pack property grid) -----------------------------
BEGIN_MESSAGE_MAP(CDetailsPane, CDockablePane)
    ON_WM_CREATE()
    ON_WM_SIZE()
END_MESSAGE_MAP()

int CDetailsPane::OnCreate(LPCREATESTRUCT createStruct) {
    if (CDockablePane::OnCreate(createStruct) == -1) {
        return -1;
    }
    grid_.Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 0, 0), this, 1);
    grid_.EnableHeaderCtrl(TRUE, T(i18n::HdrField), T(i18n::HdrValue));
    grid_.EnableDescriptionArea(FALSE);  // no room is needed for a flat detail list
    grid_.SetVSDotNetLook();
    grid_.MarkModifiedProperties(FALSE);
    return 0;
}

void CDetailsPane::OnSize(UINT type, int cx, int cy) {
    CDockablePane::OnSize(type, cx, cy);
    if (grid_.GetSafeHwnd() != nullptr) {
        grid_.SetWindowPos(nullptr, 0, 0, cx, cy, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void CDetailsPane::Show(const warehouse::StockRow& row) {
    if (grid_.GetSafeHwnd() == nullptr) {
        return;
    }
    grid_.RemoveAll();
    const auto add = [this](const CString& name, const CString& value) {
        auto* property = new CMFCPropertyGridProperty(name, COleVariant(value), nullptr);
        property->AllowEdit(FALSE);  // details are read-only
        grid_.AddProperty(property);
    };
    CString onHand;
    onHand.Format(_T("%d"), row.onHand);
    add(T(i18n::ColSku), FromUtf8(row.sku));
    add(T(i18n::ColProduct), FromUtf8(row.productName));
    add(T(i18n::ColWarehouse), FromUtf8(row.warehouseCode + " " + row.warehouseName));
    add(T(i18n::DtOnHand), onHand);
    add(T(i18n::DtLow), row.isLow ? T(i18n::YesValue) : T(i18n::NoValue));
    grid_.AdjustLayout();
}

void CDetailsPane::SetDark(bool dark) {
    if (grid_.GetSafeHwnd() == nullptr) {
        return;
    }
    if (dark) {
        grid_.SetCustomColors(RGB(30, 30, 30), RGB(220, 220, 220),    // background, text
                              RGB(45, 45, 48), RGB(220, 220, 220),    // group bg, group text
                              RGB(37, 37, 38), RGB(180, 180, 180),    // description bg, text
                              RGB(63, 63, 70));                        // grid lines
    } else {
        const COLORREF def = static_cast<COLORREF>(-1);  // -1 = framework default
        grid_.SetCustomColors(def, def, def, def, def, def, def);
    }
    grid_.AdjustLayout();
    grid_.Invalidate();
}
