#pragma once

#include <string>

namespace warehouse {

// Clean an ODBC error string for display. ODBC diagnostics arrive as
//   "context line\n[SQLSTATE][driver][server]message"
// (one or more diagnostic lines, each prefixed with bracketed [..] groups). This keeps
// only the message text of each diagnostic line — dropping the context line and the
// bracketed prefixes — so the user sees the SQL Server RAISERROR text, not driver plumbing.
//
// Operates on the raw UTF-8 bytes from std::exception::what(): the delimiters it scans for
// ('[', ']', '\n', ' ') are all ASCII, which never occur inside a UTF-8 multibyte sequence,
// so message text (incl. Polish diacritics) passes through untouched.
//
// If nothing looks like a diagnostic line, the input is returned unchanged.
std::string cleanDbError(const std::string& raw);

}  // namespace warehouse
