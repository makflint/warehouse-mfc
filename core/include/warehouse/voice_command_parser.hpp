#ifndef WAREHOUSE_VOICE_COMMAND_PARSER_HPP
#define WAREHOUSE_VOICE_COMMAND_PARSER_HPP

#include <string>

#include "warehouse/movement.hpp"

namespace warehouse {

enum class CommandKind { Unknown, RecordMovement, ShowLowStock, Refresh, Undo, Redo };

// The intent recognised from a spoken phrase. type/qty/sku are meaningful only
// when kind == RecordMovement. The app layer resolves the sku to a product id and
// builds the actual MovementCommand — the parser stays free of DB knowledge.
struct ParsedCommand {
    CommandKind kind = CommandKind::Unknown;
    MovementType type = MovementType::In;
    int qty = 0;
    std::string sku;
};

// Maps a fixed command-and-control grammar (Polish) to an intent:
//   "przyjmij <n> <sku>" -> IN     "wydaj <n> <sku>" -> OUT
//   "pokaz niskie stany" -> ShowLowStock   "odswiez" -> Refresh
//   "cofnij" -> Undo               "ponow" -> Redo
// Anything else (and malformed numbers/missing sku) -> Unknown.
ParsedCommand parseVoiceCommand(const std::string& phrase);

}  // namespace warehouse

#endif  // WAREHOUSE_VOICE_COMMAND_PARSER_HPP
