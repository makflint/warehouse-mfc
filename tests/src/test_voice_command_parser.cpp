#include "catch_amalgamated.hpp"
#include "warehouse/voice_command_parser.hpp"

using warehouse::CommandKind;
using warehouse::MovementType;
using warehouse::parseVoiceCommand;
using warehouse::ParsedCommand;

TEST_CASE("przyjmij parses to an IN movement with quantity and sku") {
    const ParsedCommand c = parseVoiceCommand("przyjmij 10 4521");
    REQUIRE(c.kind == CommandKind::RecordMovement);
    REQUIRE(c.type == MovementType::In);
    REQUIRE(c.qty == 10);
    REQUIRE(c.sku == "4521");
}

TEST_CASE("wydaj parses to an OUT movement") {
    const ParsedCommand c = parseVoiceCommand("wydaj 5 4521");
    REQUIRE(c.kind == CommandKind::RecordMovement);
    REQUIRE(c.type == MovementType::Out);
    REQUIRE(c.qty == 5);
    REQUIRE(c.sku == "4521");
}

TEST_CASE("the fixed-phrase commands are recognised") {
    REQUIRE(parseVoiceCommand("pokaż niskie stany").kind == CommandKind::ShowLowStock);
    REQUIRE(parseVoiceCommand("odśwież").kind == CommandKind::Refresh);
    REQUIRE(parseVoiceCommand("cofnij").kind == CommandKind::Undo);
    REQUIRE(parseVoiceCommand("ponów").kind == CommandKind::Redo);
}

TEST_CASE("garbage and malformed phrases map to Unknown") {
    REQUIRE(parseVoiceCommand("").kind == CommandKind::Unknown);
    REQUIRE(parseVoiceCommand("zrób mi kawę").kind == CommandKind::Unknown);
    REQUIRE(parseVoiceCommand("przyjmij dużo 4521").kind == CommandKind::Unknown);  // qty not a number
    REQUIRE(parseVoiceCommand("przyjmij 10").kind == CommandKind::Unknown);          // missing sku
    REQUIRE(parseVoiceCommand("wydaj 0 4521").kind == CommandKind::Unknown);         // non-positive qty
}
