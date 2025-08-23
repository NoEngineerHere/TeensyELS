#pragma once

#include "command_interface.h"
#include "system_commands.h"
#include "../interfaces/system_interfaces.h"

/**
 * Command-based button handler that separates UI input from business logic
 * 
 * This handler maps button presses to command objects, enabling:
 * - Clean separation between UI and business logic
 * - Undo/redo capability for supported operations
 * - Command logging and debugging
 * - Easy testing of business logic independent of UI
 */
class CommandButtonHandler : public IButtonHandler {
private:
    CommandInvoker m_commandInvoker;
    ISpindle* m_spindle;
    ILeadscrew* m_leadscrew;
    
    // Button state tracking (for debouncing and edge detection)
    struct ButtonState {
        bool currentState;
        bool previousState;
        uint32_t lastChangeTime;
        static constexpr uint32_t DEBOUNCE_MS = 50;
        
        ButtonState() : currentState(false), previousState(false), lastChangeTime(0) {}
        
        bool isPressed() const { return currentState; }
        bool wasJustPressed() const { return currentState && !previousState; }
        bool wasJustReleased() const { return !currentState && previousState; }
        
        void update(bool newState, uint32_t currentTime) {
            if (newState != currentState && (currentTime - lastChangeTime) > DEBOUNCE_MS) {
                previousState = currentState;
                currentState = newState;
                lastChangeTime = currentTime;
            }
        }
    };
    
    // Button states for different platforms
#ifdef ESP32
    // ESP32 uses button matrix - states managed by KeyArray
    ButtonState m_buttonStates[9];  // 3x3 matrix
#else
    // Teensy uses individual button pins
    ButtonState m_enableButton;
    ButtonState m_rateIncreaseButton;
    ButtonState m_rateDecreaseButton;
    ButtonState m_modeCycleButton;
    ButtonState m_threadSyncButton;
    ButtonState m_lockButton;
    ButtonState m_jogLeftButton;
    ButtonState m_jogRightButton;
#endif
    
    void updateButtonStates();
    void processButtonPresses();
    bool readButtonPin(int pin);
    
public:
    // Legacy constructor for backward compatibility
    CommandButtonHandler(ISpindle* spindle, ILeadscrew* leadscrew);
    
    // DI-aware constructor
    CommandButtonHandler(class DependencyContainer* container);
    
    // IButtonHandler interface
    void handle() override;
    
    // Command pattern specific methods
    bool undoLastAction() { return m_commandInvoker.undoLastCommand(); }
    size_t getActionHistoryCount() const { return m_commandInvoker.getHistoryCount(); }
    void clearActionHistory() { m_commandInvoker.clearHistory(); }
};

#ifdef ESP32
/**
 * ESP32-specific command button handler that works with KeyArray
 */
class ESP32CommandButtonHandler : public CommandButtonHandler {
private:
    IKeyArray* m_keyArray;
    
public:
    // Legacy constructor for backward compatibility
    ESP32CommandButtonHandler(ISpindle* spindle, ILeadscrew* leadscrew, IKeyArray* keyArray);
    
    // DI-aware constructor
    ESP32CommandButtonHandler(class DependencyContainer* container);
    
    void handle() override;
};
#endif