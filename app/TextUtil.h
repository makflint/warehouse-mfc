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
