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

TEST_CASE("recognised speech is normalised: capitalisation and punctuation tolerated") {
    // whisper.cpp returns capitalised, punctuated text (e.g. "Odśwież.") — the
    // parser lower-cases (incl. Polish letters) and strips punctuation first.
    REQUIRE(parseVoiceCommand("Odśwież.").kind == CommandKind::Refresh);
    REQUIRE(parseVoiceCommand("Cofnij!").kind == CommandKind::Undo);
    REQUIRE(parseVoiceCommand("Ponów").kind == CommandKind::Redo);
    REQUIRE(parseVoiceCommand("Pokaż niskie stany.").kind == CommandKind::ShowLowStock);

    const ParsedCommand in = parseVoiceCommand("Przyjmij 10 4521.");
    REQUIRE(in.kind == CommandKind::RecordMovement);
    REQUIRE(in.type == MovementType::In);
    REQUIRE(in.qty == 10);
    REQUIRE(in.sku == "4521");

    const ParsedCommand out = parseVoiceCommand("  Wydaj 5 4521 ");
    REQUIRE(out.kind == CommandKind::RecordMovement);
    REQUIRE(out.type == MovementType::Out);
    REQUIRE(out.qty == 5);
    REQUIRE(out.sku == "4521");
}

TEST_CASE("fuzzy match: real whisper near-misses still map to the right command") {
    // These are actual recognitions observed for this user's voice (base/small).
    REQUIRE(parseVoiceCommand("Odświesz.").kind == CommandKind::Refresh);   // ż heard as sz
    REQUIRE(parseVoiceCommand("Cofni.").kind == CommandKind::Undo);          // dropped final j
    REQUIRE(parseVoiceCommand("Ponów.").kind == CommandKind::Redo);
    REQUIRE(parseVoiceCommand("Pokaż mniskie stany.").kind == CommandKind::ShowLowStock);

    // Numbers come back spelled out and split: "Przyjmij 10 sztuk, 4, 5, 2, 1."
    const ParsedCommand c = parseVoiceCommand("Przyjmij 10 sztuk, 4, 5, 2, 1.");
    REQUIRE(c.kind == CommandKind::RecordMovement);
    REQUIRE(c.type == MovementType::In);
    REQUIRE(c.qty == 10);
    REQUIRE(c.sku == "4521");  // first number = qty, the rest concatenate into the sku
}

TEST_CASE("garbage and malformed phrases map to Unknown") {
    REQUIRE(parseVoiceCommand("").kind == CommandKind::Unknown);
    REQUIRE(parseVoiceCommand("zrób mi kawę").kind == CommandKind::Unknown);
    REQUIRE(parseVoiceCommand("przyjmij dużo 4521").kind == CommandKind::Unknown);  // qty not a number
    REQUIRE(parseVoiceCommand("przyjmij 10").kind == CommandKind::Unknown);          // missing sku
    REQUIRE(parseVoiceCommand("wydaj 0 4521").kind == CommandKind::Unknown);         // non-positive qty
}
