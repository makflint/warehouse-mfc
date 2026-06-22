#include "framework.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "DockPanes.h"
#include "TextUtil.h"
#include "WarehouseDoc.h"

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

    // Double-buffer to avoid flicker.
    CDC mem;
    mem.CreateCompatibleDC(&paint);
    CBitmap bmp;
    bmp.CreateCompatibleBitmap(&paint, rc.Width(), rc.Height());
    CBitmap* oldBmp = mem.SelectObject(&bmp);

    const COLORREF bgColor = dark_ ? RGB(30, 30, 30) : RGB(245, 246, 248);
    const COLORREF fgColor = dark_ ? RGB(215, 215, 215) : RGB(60, 60, 60);
    const COLORREF axisColor = dark_ ? RGB(90, 90, 90) : RGB(180, 180, 180);
    const COLORREF barColor = dark_ ? RGB(70, 120, 200) : RGB(40, 86, 150);
    const COLORREF barLow = dark_ ? RGB(220, 80, 80) : RGB(176, 32, 32);
    mem.FillSolidRect(rc, bgColor);

    CFont fontBig, fontSmall;
    fontBig.CreateFont(30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, _T("Segoe UI"));
    fontSmall.CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                         OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH,
                         _T("Segoe UI"));

    const std::vector<warehouse::StockRow>* stock = activeStock();

    // KPIs.
    std::map<std::string, long long> bySku;
    std::map<std::string, bool> lowBySku;
    long long totalUnits = 0;
    int lowRows = 0;
    if (stock != nullptr) {
        for (const warehouse::StockRow& row : *stock) {
            bySku[row.sku] += row.onHand;
            lowBySku[row.sku] = lowBySku[row.sku] || row.isLow;
            totalUnits += row.onHand;
            if (row.isLow) ++lowRows;
        }
    }

    const int margin = 10;
    CRect inner = rc;
    inner.DeflateRect(margin, margin);

    // --- three KPI tiles across the top ---
    const int tileH = 74;
    const int gap = 8;
    const int tileW = (inner.Width() - 2 * gap) / 3;
    CString s;
    s.Format(_T("%d"), static_cast<int>(bySku.size()));
    drawTile(mem, CRect(inner.left, inner.top, inner.left + tileW, inner.top + tileH),
             RGB(33, 115, 70), s, _T("Asortyment"), fontBig, fontSmall);
    s.Format(_T("%d"), lowRows);
    drawTile(mem, CRect(inner.left + tileW + gap, inner.top, inner.left + 2 * tileW + gap, inner.top + tileH),
             lowRows > 0 ? RGB(176, 32, 32) : RGB(120, 120, 120), s, _T("Niskie stany magazynowe"),
             fontBig, fontSmall);
    s.Format(_T("%lld"), totalUnits);
    drawTile(mem, CRect(inner.left + 2 * tileW + 2 * gap, inner.top, inner.right, inner.top + tileH),
             RGB(40, 86, 150), s, _T("Suma sztuk"), fontBig, fontSmall);

    // --- bar chart of on-hand per SKU ---
    CRect chart = inner;
    chart.top += tileH + 16;
    mem.SetBkMode(TRANSPARENT);
    mem.SelectObject(&fontSmall);
    mem.SetTextColor(fgColor);
    mem.DrawText(_T("Stan magazynowy wg indeksu"), CRect(chart.left, chart.top, chart.right, chart.top + 16),
                 DT_LEFT | DT_SINGLELINE);
    chart.top += 20;

    long long maxVal = 1;
    for (const auto& kv : bySku) maxVal = (std::max)(maxVal, kv.second);

    const int n = static_cast<int>(bySku.size());
    if (n > 0) {
        const int axisY = chart.bottom - 18;            // leave room for sku labels
        const int plotH = axisY - chart.top;
        const int slot = chart.Width() / n;
        const int barW = (std::min)(48, slot * 2 / 3);
        int i = 0;
        for (const auto& kv : bySku) {
            const int cx = chart.left + slot * i + slot / 2;
            const int h = static_cast<int>(static_cast<long long>(plotH) * kv.second / maxVal);
            CRect bar(cx - barW / 2, axisY - h, cx + barW / 2, axisY);
            const COLORREF c = lowBySku[kv.first] ? barLow : barColor;
            mem.FillSolidRect(bar, c);

            CString val;
            val.Format(_T("%lld"), kv.second);
            mem.SetTextColor(fgColor);
            mem.DrawText(val, CRect(cx - slot / 2, bar.top - 16, cx + slot / 2, bar.top),
                         DT_CENTER | DT_BOTTOM | DT_SINGLELINE);
            mem.DrawText(FromUtf8(kv.first), CRect(cx - slot / 2, axisY + 2, cx + slot / 2, axisY + 18),
                         DT_CENTER | DT_TOP | DT_SINGLELINE);
            ++i;
        }
        mem.FillSolidRect(chart.left, axisY, chart.Width(), 1, axisColor);
    }

    paint.BitBlt(0, 0, rc.Width(), rc.Height(), &mem, 0, 0, SRCCOPY);
    mem.SelectObject(oldBmp);
}

// --- Movement-log list (Czas-only sort) ------------------------------------
int CMovementLogList::OnCompareItems(LPARAM lParam1, LPARAM lParam2, int column) {
    if (rows_ == nullptr || column != 0) {  // only the Czas column sorts
        return 0;
    }
    const std::size_t a = static_cast<std::size_t>(lParam1);
    const std::size_t b = static_cast<std::size_t>(lParam2);
    if (a >= rows_->size() || b >= rows_->size()) {
        return 0;
    }
    return (*rows_)[a].createdAt.compare((*rows_)[b].createdAt);  // ISO text = chronological
}

void CMovementLogList::Sort(int column, BOOL ascending, BOOL add) {
    if (column == 0) {  // ignore clicks on the other columns (no arrow, no reorder)
        CMFCListCtrl::Sort(column, ascending, add);
    }
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
    grid_.EnableHeaderCtrl(TRUE, _T("Pole"), _T("Wartość"));
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
    add(_T("Symbol"), FromUtf8(row.sku));
    add(_T("Produkt"), FromUtf8(row.productName));
    add(_T("Magazyn"), FromUtf8(row.warehouseCode + " " + row.warehouseName));
    add(_T("Stan magazynowy"), onHand);
    add(_T("Niski stan"), row.isLow ? _T("TAK") : _T("nie"));
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
