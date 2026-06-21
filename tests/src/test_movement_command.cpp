#include "catch_amalgamated.hpp"
#include "warehouse/movement_command.hpp"

#include <vector>

using warehouse::IMovementRecorder;
using warehouse::Movement;
using warehouse::MovementCommand;
using warehouse::MovementType;

namespace {
struct FakeRecorder : IMovementRecorder {
    std::vector<Movement> recorded;
    void record(const Movement& m) override { recorded.push_back(m); }
};
}  // namespace

TEST_CASE("MovementCommand records the movement on execute") {
    FakeRecorder rec;
    MovementCommand cmd(Movement{1, 2, 5, MovementType::In}, rec);

    cmd.execute();

    REQUIRE(rec.recorded.size() == 1);
    REQUIRE(rec.recorded[0].type == MovementType::In);
    REQUIRE(rec.recorded[0].qty == 5);
}

TEST_CASE("MovementCommand records the compensating movement on undo") {
    FakeRecorder rec;
    MovementCommand cmd(Movement{1, 2, 5, MovementType::In}, rec);

    cmd.execute();
    cmd.undo();

    REQUIRE(rec.recorded.size() == 2);
    REQUIRE(rec.recorded[1].type == MovementType::Out);  // compensating IN -> OUT
    REQUIRE(rec.recorded[1].qty == 5);
}
