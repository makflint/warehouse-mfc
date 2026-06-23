#include "warehouse/db_error.hpp"

#include <cctype>

namespace warehouse {
namespace {

std::string trimmed(const std::string& s) {
    std::size_t begin = 0;
    std::size_t end = s.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(s[begin])) != 0) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1])) != 0) {
        --end;
    }
    return s.substr(begin, end - begin);
}

// Strip the leading "[..]" bracket groups (and the spaces between them) from one diagnostic
// line, returning the trailing message text.
std::string stripBracketPrefixes(const std::string& line) {
    std::size_t pos = 0;
    while (pos < line.size() && line[pos] == '[') {
        const std::size_t close = line.find(']', pos);
        if (close == std::string::npos) {
            break;  // unbalanced — leave the rest as-is
        }
        pos = close + 1;
        while (pos < line.size() && line[pos] == ' ') {
            ++pos;
        }
    }
    return trimmed(line.substr(pos));
}

}  // namespace

std::string cleanDbError(const std::string& raw) {
    std::string result;
    std::size_t start = 0;
    while (start <= raw.size()) {
        const std::size_t nl = raw.find('\n', start);
        const std::size_t lineEnd = (nl == std::string::npos) ? raw.size() : nl;
        const std::string line = trimmed(raw.substr(start, lineEnd - start));

        if (!line.empty() && line.front() == '[') {  // a diagnostic line
            const std::string text = stripBracketPrefixes(line);
            if (!text.empty()) {
                if (!result.empty()) {
                    result += '\n';
                }
                result += text;
            }
        }

        if (nl == std::string::npos) {
            break;
        }
        start = nl + 1;
    }
    return result.empty() ? raw : result;
}

}  // namespace warehouse
