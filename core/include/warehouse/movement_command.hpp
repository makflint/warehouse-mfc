#ifndef WAREHOUSE_MOVEMENT_COMMAND_HPP
#define WAREHOUSE_MOVEMENT_COMMAND_HPP

#include "warehouse/command.hpp"
#include "warehouse/movement.hpp"

namespace warehouse {

// Port implemented by the data layer (sp_RecordMovement). core/ stays DB-agnostic;
// tests substitute an in-memory fake.
struct IMovementRecorder {
    virtual ~IMovementRecorder() = default;
    virtual void record(const Movement& m) = 0;
};

// Recording a stock movement, made undoable: execute records the movement, undo
// records the compensating one (so the append-only log stays the source of truth).
class MovementCommand : public ICommand {
public:
    MovementCommand(Movement movement, IMovementRecorder& recorder);
    void execute() override;
    void undo() override;

private:
    Movement movement_;
    IMovementRecorder& recorder_;
};

}  // namespace warehouse

#endif  // WAREHOUSE_MOVEMENT_COMMAND_HPP
