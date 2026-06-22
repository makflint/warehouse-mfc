#include "warehouse/voice_command_parser.hpp"

#include <string>
#include <utility>
#include <vector>

namespace warehouse {
namespace {

// Lower-case an UTF-8 phrase, including the Polish letters whisper may capitalise.
// Each pair is the UTF-8 byte sequence of an upper-case letter and its lower-case
// form; ASCII A-Z is handled byte-wise afterwards (it never touches >=0x80 bytes).
std::string toLower(std::string text) {
    static const std::pair<const char*, const char*> kPolish[] = {
        {"\xC4\x84", "\xC4\x85"},  // Ą -> ą
        {"\xC4\x86", "\xC4\x87"},  // Ć -> ć
        {"\xC4\x98", "\xC4\x99"},  // Ę -> ę
        {"\xC5\x81", "\xC5\x82"},  // Ł -> ł
        {"\xC5\x83", "\xC5\x84"},  // Ń -> ń
        {"\xC3\x93", "\xC3\xB3"},  // Ó -> ó
        {"\xC5\x9A", "\xC5\x9B"},  // Ś -> ś
        {"\xC5\xB9", "\xC5\xBA"},  // Ź -> ź
        {"\xC5\xBB", "\xC5\xBC"},  // Ż -> ż
    };
    for (const auto& [upper, lower] : kPolish) {
        for (std::size_t at = text.find(upper); at != std::string::npos; at = text.find(upper, at)) {
            text.replace(at, 2, lower);
        }
    }
    for (char& c : text) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return text;
}

// Fold lower-case Polish letters to their closest ASCII letter, so keyword matching
// is robust to whisper hearing "odświesz" for "odśwież" etc. Run after toLower.
std::string asciiFold(std::string text) {
    static const std::pair<const char*, char> kFold[] = {
        {"\xC4\x85", 'a'},  // ą
        {"\xC4\x87", 'c'},  // ć
        {"\xC4\x99", 'e'},  // ę
        {"\xC5\x82", 'l'},  // ł
        {"\xC5\x84", 'n'},  // ń
        {"\xC3\xB3", 'o'},  // ó
        {"\xC5\x9B", 's'},  // ś
        {"\xC5\xBA", 'z'},  // ź
        {"\xC5\xBC", 'z'},  // ż
    };
    for (const auto& [from, to] : kFold) {
        for (std::size_t at = text.find(from); at != std::string::npos; at = text.find(from, at)) {
            text.replace(at, 2, 1, to);
        }
    }
    return text;
}

// Digit runs in order, e.g. "10 sztuk 4 5 2 1" -> {"10","4","5","2","1"}.
std::vector<std::string> digitGroups(const std::string& text) {
    std::vector<std::string> groups;
    std::string current;
    for (const char c : text) {
        if (c >= '0' && c <= '9') {
            current += c;
        } else if (!current.empty()) {
            groups.push_back(current);
            current.clear();
        }
    }
    if (!current.empty()) {
        groups.push_back(current);
    }
    return groups;
}

bool parsePositiveInt(const std::string& text, int& out) {
    if (text.empty()) {
        return false;
    }
    try {
        const int value = std::stoi(text);
        if (value <= 0) {
            return false;
        }
        out = value;
        return true;
    } catch (...) {
        return false;  // out of int range
    }
}

// "przyjmij 10 4521" -> qty from the first number, sku from the rest concatenated
// ("Przyjmij 10 sztuk, 4, 5, 2, 1" -> qty 10, sku 4521). Needs at least two numbers.
ParsedCommand recordMovement(MovementType type, const std::string& text) {
    const std::vector<std::string> numbers = digitGroups(text);
    int qty = 0;
    if (numbers.size() < 2 || !parsePositiveInt(numbers[0], qty)) {
        return ParsedCommand{};
    }
    ParsedCommand command;
    command.kind = CommandKind::RecordMovement;
    command.type = type;
    command.qty = qty;
    for (std::size_t i = 1; i < numbers.size(); ++i) {
        command.sku += numbers[i];
    }
    return command;
}

}  // namespace

// Command-and-control is forgiving: whisper rarely returns the exact phrase, so we
// match on ASCII-folded keyword stems rather than exact tokens. Order matters —
// the movement verbs are checked first because they also carry the numbers.
ParsedCommand parseVoiceCommand(const std::string& phrase) {
    const std::string text = asciiFold(toLower(phrase));
    const auto has = [&text](const char* stem) { return text.find(stem) != std::string::npos; };

    if (has("wyda")) return recordMovement(MovementType::Out, text);
    if (has("przyj")) return recordMovement(MovementType::In, text);
    if (has("nisk")) return {CommandKind::ShowLowStock};
    if (has("cofn")) return {CommandKind::Undo};
    if (has("ponow")) return {CommandKind::Redo};
    if (has("odsw")) return {CommandKind::Refresh};
    return ParsedCommand{};
}

}  // namespace warehouse
