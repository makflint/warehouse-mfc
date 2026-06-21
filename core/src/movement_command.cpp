#include "warehouse/movement_command.hpp"

namespace warehouse {

MovementCommand::MovementCommand(Movement movement, IMovementRecorder& recorder)
    : movement_(movement), recorder_(recorder) {}

void MovementCommand::execute() {
    recorder_.record(movement_);
}

void MovementCommand::undo() {
    recorder_.record(compensating(movement_));
}

}  // namespace warehouse
