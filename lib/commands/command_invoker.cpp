#include "command_interface.h"

bool CommandInvoker::executeCommand(ICommand* command) {
    if (!command) {
        return false;
    }
    
    bool success = command->execute();
    
    if (success && command->isUndoable()) {
        // Add to history (circular buffer)
        m_commandHistory[m_historyIndex] = command;
        m_historyIndex = (m_historyIndex + 1) % MAX_HISTORY;
        
        if (m_historyCount < MAX_HISTORY) {
            m_historyCount++;
        }
    } else {
        // Command not undoable or failed, don't add to history
        // Note: Command ownership not transferred if not added to history
    }
    
    return success;
}

bool CommandInvoker::undoLastCommand() {
    if (m_historyCount == 0) {
        return false;
    }
    
    // Get the most recent command
    size_t lastIndex = (m_historyIndex - 1 + MAX_HISTORY) % MAX_HISTORY;
    ICommand* lastCommand = m_commandHistory[lastIndex];
    
    if (!lastCommand || !lastCommand->isUndoable()) {
        return false;
    }
    
    bool success = lastCommand->undo();
    
    if (success) {
        m_commandHistory[lastIndex] = nullptr;
        m_historyIndex = lastIndex;
        m_historyCount--;
    }
    
    return success;
}

void CommandInvoker::clearHistory() {
    for (size_t i = 0; i < MAX_HISTORY; ++i) {
        m_commandHistory[i] = nullptr;
    }
    m_historyCount = 0;
    m_historyIndex = 0;
}