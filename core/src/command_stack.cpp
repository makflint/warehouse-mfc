#include "warehouse/command.hpp"

namespace warehouse {

void CommandStack::execute(std::unique_ptr<ICommand> command) {
    command->execute();
    undo_.push_back(std::move(command));
    redo_.clear();  // a new action invalidates the redo branch
}

void CommandStack::undo() {
    if (!canUndo()) {
        return;
    }
    std::unique_ptr<ICommand> command = std::move(undo_.back());
    undo_.pop_back();
    command->undo();
    redo_.push_back(std::move(command));
}

void CommandStack::redo() {
    if (!canRedo()) {
        return;
    }
    std::unique_ptr<ICommand> command = std::move(redo_.back());
    redo_.pop_back();
    command->execute();
    undo_.push_back(std::move(command));
}

}  // namespace warehouse
