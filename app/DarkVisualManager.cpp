#include "framework.h"

#include "DarkVisualManager.h"

IMPLEMENT_DYNCREATE(CDarkVisualManager, CMFCVisualManagerOffice2007)

namespace {
constexpr COLORREF kHeaderBg = RGB(45, 45, 48);
constexpr COLORREF kHeaderPressed = RGB(30, 30, 32);
constexpr COLORREF kHeaderHot = RGB(62, 62, 66);
constexpr COLORREF kHeaderBorder = RGB(63, 63, 70);
constexpr COLORREF kArrow = RGB(220, 220, 220);
}  // namespace

void CDarkVisualManager::OnFillHeaderCtrlBackground(CMFCHeaderCtrl* /*ctrl*/, CDC* dc, CRect rect) {
    dc->FillSolidRect(rect, kHeaderBg);
}

void CDarkVisualManager::OnDrawHeaderCtrlBorder(CMFCHeaderCtrl* /*ctrl*/, CDC* dc, CRect& rect,
                                                BOOL isPressed, BOOL isHighlighted) {
    const COLORREF fill = isPressed ? kHeaderPressed : (isHighlighted ? kHeaderHot : kHeaderBg);
    dc->FillSolidRect(rect, fill);
    dc->Draw3dRect(rect, kHeaderBorder, kHeaderBorder);
}

void CDarkVisualManager::OnDrawHeaderCtrlSortArrow(CMFCHeaderCtrl* /*ctrl*/, CDC* dc, CRect& rect,
                                                   BOOL isUp) {
    const int cx = rect.left + rect.Width() / 2;
    CPoint points[3];
    if (isUp) {
        points[0] = CPoint(cx, rect.top);
        points[1] = CPoint(rect.left, rect.bottom);
        points[2] = CPoint(rect.right, rect.bottom);
    } else {
        points[0] = CPoint(rect.left, rect.top);
        points[1] = CPoint(rect.right, rect.top);
        points[2] = CPoint(cx, rect.bottom);
    }
    CBrush brush(kArrow);
    CPen pen(PS_SOLID, 1, kArrow);
    CBrush* oldBrush = dc->SelectObject(&brush);
    CPen* oldPen = dc->SelectObject(&pen);
    dc->Polygon(points, 3);
    dc->SelectObject(oldBrush);
    dc->SelectObject(oldPen);
}
