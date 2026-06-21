#include "warehouse/voice_command_parser.hpp"

#include <cctype>
#include <sstream>
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

// Replace ASCII punctuation (whisper adds trailing "." / "!" etc.) with spaces, so
// tokenisation drops it. UTF-8 letter bytes (>=0x80) are left untouched.
std::string stripPunctuation(std::string text) {
    for (char& c : text) {
        const unsigned char byte = static_cast<unsigned char>(c);
        if (byte < 0x80 && std::isalnum(byte) == 0) {
            c = ' ';
        }
    }
    return text;
}

std::vector<std::string> tokenize(const std::string& phrase) {
    std::istringstream in(phrase);
    std::vector<std::string> tokens;
    std::string token;
    while (in >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// A spoken quantity is a positive whole number ("przyjmij 10 ...").
bool parsePositiveInt(const std::string& text, int& out) {
    if (text.empty()) {
        return false;
    }
    for (const char c : text) {
        if (c < '0' || c > '9') {
            return false;
        }
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

ParsedCommand recordMovement(MovementType type, const std::vector<std::string>& tokens) {
    int qty = 0;
    if (!parsePositiveInt(tokens[1], qty) || tokens[2].empty()) {
        return ParsedCommand{};
    }
    ParsedCommand command;
    command.kind = CommandKind::RecordMovement;
    command.type = type;
    command.qty = qty;
    command.sku = tokens[2];
    return command;
}

}  // namespace

ParsedCommand parseVoiceCommand(const std::string& phrase) {
    const std::vector<std::string> tokens = tokenize(stripPunctuation(toLower(phrase)));

    if (tokens.size() == 1) {
        if (tokens[0] == "odśwież") return {CommandKind::Refresh};
        if (tokens[0] == "cofnij")  return {CommandKind::Undo};
        if (tokens[0] == "ponów")   return {CommandKind::Redo};
    } else if (tokens.size() == 3) {
        if (tokens[0] == "pokaż" && tokens[1] == "niskie" && tokens[2] == "stany") {
            return {CommandKind::ShowLowStock};
        }
        if (tokens[0] == "przyjmij") return recordMovement(MovementType::In, tokens);
        if (tokens[0] == "wydaj")    return recordMovement(MovementType::Out, tokens);
    }
    return ParsedCommand{};
}

}  // namespace warehouse
