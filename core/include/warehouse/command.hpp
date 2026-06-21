#ifndef WAREHOUSE_COMMAND_HPP
#define WAREHOUSE_COMMAND_HPP

#include <memory>
#include <vector>

namespace warehouse {

// Command pattern: every user action that can be undone implements this. The
// undo/redo history (CommandStack) only ever sees this interface.
struct ICommand {
    virtual ~ICommand() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
};

// Holds the undo/redo history. Executing a fresh command discards any redo branch
// (standard linear undo). Owns the commands it has run.
class CommandStack {
public:
    void execute(std::unique_ptr<ICommand> command);
    void undo();
    void redo();

    bool canUndo() const noexcept { return !undo_.empty(); }
    bool canRedo() const noexcept { return !redo_.empty(); }

private:
    std::vector<std::unique_ptr<ICommand>> undo_;
    std::vector<std::unique_ptr<ICommand>> redo_;
};

}  // namespace warehouse

#endif  // WAREHOUSE_COMMAND_HPP
