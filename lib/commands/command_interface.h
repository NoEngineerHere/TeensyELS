#pragma once

#include <cstddef>

/**
 * Command pattern interface for UI/business logic separation
 * 
 * This decouples user interactions from business logic by encapsulating
 * operations as command objects that can be executed, queued, or logged.
 */

class ICommand {
public:
    virtual ~ICommand() = default;
    
    /**
     * Execute the command
     * @return true if command executed successfully, false otherwise
     */
    virtual bool execute() = 0;
    
    /**
     * Undo the command if possible
     * @return true if command was undone successfully, false if not undoable
     */
    virtual bool undo() { return false; }
    
    /**
     * Check if this command can be undone
     * @return true if undoable, false otherwise
     */
    virtual bool isUndoable() const { return false; }
    
    /**
     * Get human-readable description of the command
     * @return command description for logging/debugging
     */
    virtual const char* getDescription() const = 0;
};

/**
 * Command invoker that executes commands and optionally maintains history
 */
class CommandInvoker {
private:
    static constexpr size_t MAX_HISTORY = 10;
    ICommand* m_commandHistory[MAX_HISTORY];
    size_t m_historyCount;
    size_t m_historyIndex;
    
public:
    CommandInvoker() : m_historyCount(0), m_historyIndex(0) {
        for (size_t i = 0; i < MAX_HISTORY; ++i) {
            m_commandHistory[i] = nullptr;
        }
    }
    
    /**
     * Execute a command and add to history if successful
     * @param command Command to execute (ownership transferred)
     * @return true if executed successfully
     */
    bool executeCommand(ICommand* command);
    
    /**
     * Undo the last executed command
     * @return true if undo was successful
     */
    bool undoLastCommand();
    
    /**
     * Get the number of commands in history
     * @return number of commands that can be undone
     */
    size_t getHistoryCount() const { return m_historyCount; }
    
    /**
     * Clear command history
     */
    void clearHistory();
};