#include "warehouse/voice_command_parser.hpp"

#include <sstream>
#include <vector>

namespace warehouse {
namespace {

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
    const std::vector<std::string> tokens = tokenize(phrase);

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
