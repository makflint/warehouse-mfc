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
    dc.SelectObject(&smallFont);
    dc.DrawText(label, bottom, DT_CENTER | DT_TOP | DT_SINGLELINE);
    dc.SelectObject(old);
}

}  // namespace

// --- Dashboard pane --------------------------------------------------------
BEGIN_MESSAGE_MAP(CDashboardPane, CDockablePane)
    ON_WM_PAINT()
END_MESSAGE_MAP()

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
    mem.FillSolidRect(rc, RGB(245, 246, 248));

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
    const int tileH = 64;
    const int gap = 8;
    const int tileW = (inner.Width() - 2 * gap) / 3;
    CString s;
    s.Format(_T("%d"), static_cast<int>(bySku.size()));
    drawTile(mem, CRect(inner.left, inner.top, inner.left + tileW, inner.top + tileH),
             RGB(33, 115, 70), s, _T("Indeksy (SKU)"), fontBig, fontSmall);
    s.Format(_T("%d"), lowRows);
    drawTile(mem, CRect(inner.left + tileW + gap, inner.top, inner.left + 2 * tileW + gap, inner.top + tileH),
             lowRows > 0 ? RGB(176, 32, 32) : RGB(120, 120, 120), s, _T("Niskie stany"), fontBig, fontSmall);
    s.Format(_T("%lld"), totalUnits);
    drawTile(mem, CRect(inner.left + 2 * tileW + 2 * gap, inner.top, inner.right, inner.top + tileH),
             RGB(40, 86, 150), s, _T("Suma sztuk"), fontBig, fontSmall);

    // --- bar chart of on-hand per SKU ---
    CRect chart = inner;
    chart.top += tileH + 16;
    mem.SetBkMode(TRANSPARENT);
    mem.SelectObject(&fontSmall);
    mem.SetTextColor(RGB(60, 60, 60));
    mem.DrawText(_T("Stan na rękę wg indeksu"), CRect(chart.left, chart.top, chart.right, chart.top + 16),
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
            const COLORREF c = lowBySku[kv.first] ? RGB(176, 32, 32) : RGB(40, 86, 150);
            mem.FillSolidRect(bar, c);

            CString val;
            val.Format(_T("%lld"), kv.second);
            mem.SetTextColor(RGB(60, 60, 60));
            mem.DrawText(val, CRect(cx - slot / 2, bar.top - 16, cx + slot / 2, bar.top),
                         DT_CENTER | DT_BOTTOM | DT_SINGLELINE);
            mem.DrawText(FromUtf8(kv.first), CRect(cx - slot / 2, axisY + 2, cx + slot / 2, axisY + 18),
                         DT_CENTER | DT_TOP | DT_SINGLELINE);
            ++i;
        }
        mem.FillSolidRect(chart.left, axisY, chart.Width(), 1, RGB(180, 180, 180));
    }

    paint.BitBlt(0, 0, rc.Width(), rc.Height(), &mem, 0, 0, SRCCOPY);
    mem.SelectObject(oldBmp);
}

// --- List pane (Movement log / Details) ------------------------------------
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
    return 0;
}

void CListPane::OnSize(UINT type, int cx, int cy) {
    CDockablePane::OnSize(type, cx, cy);
    if (list_.GetSafeHwnd() != nullptr) {
        list_.SetWindowPos(nullptr, 0, 0, cx, cy, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}
