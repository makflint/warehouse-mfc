#include <string>

#include "catch_amalgamated.hpp"
#include "warehouse/db_error.hpp"

using warehouse::cleanDbError;

TEST_CASE("cleanDbError strips ODBC bracket prefixes and the context line") {
    const std::string raw =
        "StockRepository::recordMovement failed\n"
        "[42000][Microsoft][ODBC Driver 17 for SQL Server][SQL Server]"
        "Insufficient stock for the issue (OUT).";
    REQUIRE(cleanDbError(raw) == "Insufficient stock for the issue (OUT).");
}

TEST_CASE("cleanDbError preserves Polish UTF-8 message text") {
    const std::string raw =
        "[42000][Microsoft][SQL Server]Brak wystarczaj\xC4\x85" "cego stanu dla wydania (OUT).";
    REQUIRE(cleanDbError(raw) ==
            "Brak wystarczaj\xC4\x85" "cego stanu dla wydania (OUT).");
}

TEST_CASE("cleanDbError joins multiple diagnostic lines with newlines") {
    const std::string raw =
        "[01000][driver]First diagnostic.\n"
        "[42000][driver]Second diagnostic.";
    REQUIRE(cleanDbError(raw) == "First diagnostic.\nSecond diagnostic.");
}

TEST_CASE("cleanDbError returns the input unchanged when there is no diagnostic line") {
    REQUIRE(cleanDbError("plain message, no brackets") == "plain message, no brackets");
    REQUIRE(cleanDbError("") == "");
}

TEST_CASE("cleanDbError skips a diagnostic line that is only prefixes") {
    const std::string raw =
        "[01000][driver]\n"
        "[42000][driver]Real message.";
    REQUIRE(cleanDbError(raw) == "Real message.");
}

TEST_CASE("cleanDbError trims whitespace around a diagnostic line") {
    REQUIRE(cleanDbError("   [42000][driver]Padded message.   ") == "Padded message.");
}

TEST_CASE("cleanDbError skips spaces between bracket groups") {
    REQUIRE(cleanDbError("[A] [B]Spaced prefixes.") == "Spaced prefixes.");
}

TEST_CASE("cleanDbError leaves an unbalanced bracket line unchanged") {
    REQUIRE(cleanDbError("[unterminated message") == "[unterminated message");
}
