#pragma once

#include "command_interface.h"
#include "../interfaces/system_interfaces.h"
#include <globalstate.h>

// Ensure enum types are available
using MotionMode = GlobalMotionMode;
using FeedMode = GlobalFeedMode;
using LockMode = GlobalButtonLock;
using SyncMode = GlobalThreadSyncState;

/**
 * System commands for common ELS operations
 * These encapsulate business logic operations initiated by user interactions
 */

class SetMotionModeCommand : public ICommand {
private:
    MotionMode m_newMode;
    MotionMode m_previousMode;
    
public:
    SetMotionModeCommand(MotionMode newMode) 
        : m_newMode(newMode), m_previousMode(MotionMode::UNSET) {}
    
    bool execute() override {
        auto globalState = GlobalState::getInstance();
        m_previousMode = globalState->getMotionMode();
        globalState->setMotionMode(m_newMode);
        return true;
    }
    
    bool undo() override {
        if (m_previousMode != MotionMode::UNSET) {
            GlobalState::getInstance()->setMotionMode(m_previousMode);
            return true;
        }
        return false;
    }
    
    bool isUndoable() const override { return true; }
    
    const char* getDescription() const override {
        switch (m_newMode) {
            case MotionMode::MM_DISABLED: return "Disable motion";
            case MotionMode::MM_ENABLED: return "Enable motion";
            case MotionMode::JOG_LEFT: return "Set jog left mode";
            case MotionMode::JOG_RIGHT: return "Set jog right mode";
            default: return "Set motion mode";
        }
    }
};

class CycleFeedModeCommand : public ICommand {
private:
    FeedMode m_previousMode;
    
public:
    CycleFeedModeCommand() : m_previousMode(FeedMode::UNSET) {}
    
    bool execute() override {
        auto globalState = GlobalState::getInstance();
        m_previousMode = globalState->getFeedMode();
        
        FeedMode newMode = (m_previousMode == FeedMode::THREAD) ? 
                          FeedMode::FEED : FeedMode::THREAD;
        globalState->setFeedMode(newMode);
        return true;
    }
    
    bool undo() override {
        if (m_previousMode != FeedMode::UNSET) {
            GlobalState::getInstance()->setFeedMode(m_previousMode);
            return true;
        }
        return false;
    }
    
    bool isUndoable() const override { return true; }
    
    const char* getDescription() const override {
        return "Cycle feed mode (Thread/Feed)";
    }
};

class AdjustPitchCommand : public ICommand {
private:
    ILeadscrew* m_leadscrew;
    bool m_increase;
    int m_previousSelect;
    
public:
    AdjustPitchCommand(ILeadscrew* leadscrew, bool increase) 
        : m_leadscrew(leadscrew), m_increase(increase), m_previousSelect(-1) {}
    
    bool execute() override {
        if (!m_leadscrew) return false;
        
        auto globalState = GlobalState::getInstance();
        m_previousSelect = globalState->getFeedSelect();
        
        if (m_increase) {
            globalState->nextFeedPitch();
        } else {
            globalState->prevFeedPitch();
        }
        
        float newPitch = globalState->getCurrentFeedPitch();
        m_leadscrew->setTargetPitchMM(newPitch);
        return true;
    }
    
    bool undo() override {
        if (m_previousSelect >= 0 && m_leadscrew) {
            auto globalState = GlobalState::getInstance();
            globalState->setFeedSelect(m_previousSelect);
            float pitch = globalState->getCurrentFeedPitch();
            m_leadscrew->setTargetPitchMM(pitch);
            return true;
        }
        return false;
    }
    
    bool isUndoable() const override { return true; }
    
    const char* getDescription() const override {
        return m_increase ? "Increase pitch" : "Decrease pitch";
    }
};

class ToggleLockCommand : public ICommand {
private:
    LockMode m_previousLock;
    
public:
    ToggleLockCommand() : m_previousLock(LockMode::UNSET) {}
    
    bool execute() override {
        auto globalState = GlobalState::getInstance();
        m_previousLock = globalState->getButtonLock();
        
        LockMode newLock = (m_previousLock == LockMode::LOCKED) ? 
                          LockMode::UNLOCKED : LockMode::LOCKED;
        globalState->setButtonLock(newLock);
        return true;
    }
    
    bool undo() override {
        if (m_previousLock != LockMode::UNSET) {
            GlobalState::getInstance()->setButtonLock(m_previousLock);
            return true;
        }
        return false;
    }
    
    bool isUndoable() const override { return true; }
    
    const char* getDescription() const override {
        return "Toggle lock mode";
    }
};

class ThreadSyncCommand : public ICommand {
private:
    ISpindle* m_spindle;
    ILeadscrew* m_leadscrew;
    
public:
    ThreadSyncCommand(ISpindle* spindle, ILeadscrew* leadscrew) 
        : m_spindle(spindle), m_leadscrew(leadscrew) {}
    
    bool execute() override {
        if (!m_spindle || !m_leadscrew) return false;
        
        // Synchronize leadscrew position with spindle for threading
        m_leadscrew->setCurrentPosition(0);  // Reset leadscrew position for sync
        
        auto globalState = GlobalState::getInstance();
        globalState->setThreadSyncState(SyncMode::SYNC);
        
        return true;
    }
    
    bool undo() override {
        // Thread sync cannot be undone - it's a reset operation
        return false;
    }
    
    bool isUndoable() const override { return false; }
    
    const char* getDescription() const override {
        return "Synchronize thread position";
    }
};

class JogCommand : public ICommand {
private:
    ILeadscrew* m_leadscrew;
    bool m_jogLeft;
    
public:
    JogCommand(ILeadscrew* leadscrew, bool jogLeft) 
        : m_leadscrew(leadscrew), m_jogLeft(jogLeft) {}
    
    bool execute() override {
        if (!m_leadscrew) return false;
        
        auto globalState = GlobalState::getInstance();
        
        // Set jog mode if not already set
        MotionMode jogMode = m_jogLeft ? MotionMode::JOG_LEFT : MotionMode::JOG_RIGHT;
        if (globalState->getMotionMode() != jogMode) {
            globalState->setMotionMode(jogMode);
        }
        
        // Jog direction is handled by the leadscrew update logic
        // The motion mode change triggers the appropriate behavior
        return true;
    }
    
    bool undo() override {
        // Jogging cannot be undone - it's a movement command
        return false;
    }
    
    bool isUndoable() const override { return false; }
    
    const char* getDescription() const override {
        return m_jogLeft ? "Jog left" : "Jog right";
    }
};