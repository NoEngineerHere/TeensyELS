#include "command_button_handler.h"
#include "../platform/platform_abstraction.h"
#include <config.h>
#include <globalstate.h>

#ifdef PIO_UNIT_TESTING
#include "../../test/arduino_test_mock.h"
#else
#include <Arduino.h>
#endif

CommandButtonHandler::CommandButtonHandler(ISpindle* spindle, ILeadscrew* leadscrew)
    : m_spindle(spindle), m_leadscrew(leadscrew) {
}

void CommandButtonHandler::handle() {
    updateButtonStates();
    processButtonPresses();
}

void CommandButtonHandler::updateButtonStates() {
    uint32_t currentTime = millis();
    
#ifdef ESP32
    // ESP32 button matrix handling is done in ESP32CommandButtonHandler
#else
    // Teensy individual button pins
    m_enableButton.update(readButtonPin(ELS_ENABLE_BUTTON), currentTime);
    m_rateIncreaseButton.update(readButtonPin(ELS_RATE_INCREASE_BUTTON), currentTime);
    m_rateDecreaseButton.update(readButtonPin(ELS_RATE_DECREASE_BUTTON), currentTime);
    m_modeCycleButton.update(readButtonPin(ELS_MODE_CYCLE_BUTTON), currentTime);
    m_threadSyncButton.update(readButtonPin(ELS_THREAD_SYNC_BUTTON), currentTime);
    m_lockButton.update(readButtonPin(ELS_LOCK_BUTTON), currentTime);
    m_jogLeftButton.update(readButtonPin(ELS_JOG_LEFT_BUTTON), currentTime);
    m_jogRightButton.update(readButtonPin(ELS_JOG_RIGHT_BUTTON), currentTime);
#endif
}

void CommandButtonHandler::processButtonPresses() {
#ifdef ESP32
    // ESP32 button processing is handled in ESP32CommandButtonHandler
#else
    // Teensy button processing
    if (m_enableButton.wasJustPressed()) {
        auto globalState = GlobalState::getInstance();
        MotionMode currentMode = globalState->getMotionMode();
        MotionMode newMode = (currentMode == MotionMode::MM_DISABLED) ? 
                            MotionMode::MM_ENABLED : MotionMode::MM_DISABLED;
        
        auto command = new SetMotionModeCommand(newMode);
        m_commandInvoker.executeCommand(command);
    }
    
    if (m_rateIncreaseButton.wasJustPressed()) {
        auto command = new AdjustPitchCommand(m_leadscrew, true);
        m_commandInvoker.executeCommand(command);
    }
    
    if (m_rateDecreaseButton.wasJustPressed()) {
        auto command = new AdjustPitchCommand(m_leadscrew, false);
        m_commandInvoker.executeCommand(command);
    }
    
    if (m_modeCycleButton.wasJustPressed()) {
        auto command = new CycleFeedModeCommand();
        m_commandInvoker.executeCommand(command);
    }
    
    if (m_threadSyncButton.wasJustPressed()) {
        auto command = new ThreadSyncCommand(m_spindle, m_leadscrew);
        m_commandInvoker.executeCommand(command);
    }
    
    if (m_lockButton.wasJustPressed()) {
        auto command = new ToggleLockCommand();
        m_commandInvoker.executeCommand(command);
    }
    
    if (m_jogLeftButton.wasJustPressed()) {
        auto command = new JogCommand(m_leadscrew, true);
        m_commandInvoker.executeCommand(command);
    }
    
    if (m_jogRightButton.wasJustPressed()) {
        auto command = new JogCommand(m_leadscrew, false);
        m_commandInvoker.executeCommand(command);
    }
#endif
}

bool CommandButtonHandler::readButtonPin(int pin) {
    // Buttons are active LOW with internal pull-ups
    return digitalRead(pin) == LOW;
}

#ifdef ESP32
ESP32CommandButtonHandler::ESP32CommandButtonHandler(ISpindle* spindle, ILeadscrew* leadscrew, IKeyArray* keyArray)
    : CommandButtonHandler(spindle, leadscrew), m_keyArray(keyArray) {
}

void ESP32CommandButtonHandler::handle() {
    if (!m_keyArray) {
        return;
    }
    
    // ESP32 KeyArray-based button handling
    // The KeyArray manages the button matrix and provides button state
    // This implementation would need to interface with the existing KeyArray system
    
    // For now, call the base class to maintain compatibility
    CommandButtonHandler::handle();
    
    // TODO: Implement ESP32-specific button matrix handling using KeyArray
    // This would involve:
    // 1. Getting button states from KeyArray
    // 2. Converting matrix positions to logical button functions
    // 3. Creating appropriate commands based on button presses
}
#endif