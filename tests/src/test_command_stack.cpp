#include "catch_amalgamated.hpp"
#include "warehouse/command.hpp"

#include <memory>
#include <string>
#include <vector>

using warehouse::CommandStack;
using warehouse::ICommand;

namespace {
// Records each execute/undo call so tests can assert the exact replay order.
struct RecordingCommand : ICommand {
    std::vector<std::string>& log;
    std::string name;
    RecordingCommand(std::vector<std::string>& l, std::string n)
        : log(l), name(std::move(n)) {}
    void execute() override { log.push_back(name + ":exec"); }
    void undo() override { log.push_back(name + ":undo"); }
};

std::unique_ptr<ICommand> make(std::vector<std::string>& log, std::string name) {
    return std::make_unique<RecordingCommand>(log, std::move(name));
}
}  // namespace

TEST_CASE("CommandStack runs a command and exposes undo availability") {
    std::vector<std::string> log;
    CommandStack stack;
    REQUIRE_FALSE(stack.canUndo());
    REQUIRE_FALSE(stack.canRedo());

    stack.execute(make(log, "A"));

    REQUIRE(log == std::vector<std::string>{"A:exec"});
    REQUIRE(stack.canUndo());
    REQUIRE_FALSE(stack.canRedo());
}

TEST_CASE("CommandStack undo then redo replays the command") {
    std::vector<std::string> log;
    CommandStack stack;
    stack.execute(make(log, "A"));

    stack.undo();
    REQUIRE(stack.canRedo());
    REQUIRE_FALSE(stack.canUndo());

    stack.redo();
    REQUIRE(log == std::vector<std::string>{"A:exec", "A:undo", "A:exec"});
    REQUIRE(stack.canUndo());
    REQUIRE_FALSE(stack.canRedo());
}

TEST_CASE("executing a fresh command discards the redo branch") {
    std::vector<std::string> log;
    CommandStack stack;
    stack.execute(make(log, "A"));
    stack.undo();
    REQUIRE(stack.canRedo());

    stack.execute(make(log, "B"));

    REQUIRE_FALSE(stack.canRedo());
    REQUIRE(log == std::vector<std::string>{"A:exec", "A:undo", "B:exec"});
}

TEST_CASE("undo/redo with nothing to do are safe no-ops") {
    CommandStack stack;
    REQUIRE_NOTHROW(stack.undo());
    REQUIRE_NOTHROW(stack.redo());
}
