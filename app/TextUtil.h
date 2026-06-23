#pragma once

#include <string>

#include "framework.h"

// Caption for the app's message boxes — AfxMessageBox would otherwise title them
// with the exe name ("app"). Use as the explicit caption in MessageBox() calls.
constexpr LPCTSTR kAppTitle = _T("Stany magazynowe");

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

// Low-stock accent used by the grid rows and the dashboard bars (light theme).
constexpr COLORREF kLowStockColor = RGB(192, 0, 0);

// Content palette for the optional dark theme; the light theme uses framework
// defaults. Drives the grids, dock list and dashboard so they match the dark chrome.
struct ThemeColors {
    COLORREF background;
    COLORREF text;
    COLORREF lowStock;
};

inline ThemeColors ThemeColorsFor(bool dark) {
    if (dark) {
        return {RGB(30, 30, 30), RGB(220, 220, 220), RGB(255, 110, 110)};
    }
    return {::GetSysColor(COLOR_WINDOW), ::GetSysColor(COLOR_WINDOWTEXT), kLowStockColor};
}
