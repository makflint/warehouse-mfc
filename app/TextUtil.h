#pragma once

#include <string>

#include "framework.h"

// Convert a UTF-8 std::string (as produced by the data layer) to a CString.
// The build is Unicode, so CString is wide; a plain CString(const char*) would
// reinterpret the bytes in the ANSI code page and mangle Polish characters.
inline CString FromUtf8(const std::string& text) {
    if (text.empty()) {
        return CString();
    }
    const int length =
        ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    CString result;
    ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                          result.GetBuffer(length), length);
    result.ReleaseBuffer(length);
    return result;
}

// The system UI font (Segoe UI on Win10/11), so list controls match the
// ribbon, dashboard and status bar instead of the dated default GUI font.
inline void CreateUiFont(CFont& font) {
    NONCLIENTMETRICS metrics = {};
    metrics.cbSize = sizeof(metrics);
    if (::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0)) {
        font.CreateFontIndirect(&metrics.lfMessageFont);
    }
}

// The content-area palette for the current theme. Standard list controls aren't
// themed by CMFCVisualManager, so we recolour them (and the owner-drawn dashboard)
// by hand to follow the dark/light toggle.
struct ThemeColors {
    COLORREF background;
    COLORREF text;
    COLORREF lowStock;  // low-stock accent (grid rows, dashboard bars)
};

inline ThemeColors ThemeColorsFor(bool dark) {
    if (dark) {
        return {RGB(37, 37, 38), RGB(240, 240, 240), RGB(255, 110, 110)};
    }
    return {RGB(255, 255, 255), RGB(0, 0, 0), RGB(192, 0, 0)};
}

// Recolour a report list to match the theme (no-op until the control exists).
inline void ApplyListTheme(CListCtrl& list, bool dark) {
    if (list.GetSafeHwnd() == nullptr) {
        return;
    }
    const ThemeColors colors = ThemeColorsFor(dark);
    list.SetBkColor(colors.background);
    list.SetTextBkColor(colors.background);
    list.SetTextColor(colors.text);
    list.Invalidate();
}
