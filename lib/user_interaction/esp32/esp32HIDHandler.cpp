#include <config.h>

#if ELS_BOARD == ELS_BOARD_ESP32

#include "esp32HIDHandler.h"

#include <Arduino.h>
#include <ESP32Encoder.h>
#include <globalstate.h>
#include <leadscrew.h>

// ESP32 hardware encoder library only supports up to 8 encoders
// The total encoder count is checked at compile time via ELS_TOTAL_ENCODERS macro
// This includes:
// - 1 spindle encoder (when ELS_SPINDLE_DRIVEN is not defined)
// - UI encoders from ENCODER_DEFINITIONS array

/**
 * @brief      Compile-time check to ensure encoder count doesn't exceed ESP32 limits
 * @note       ESP32 hardware encoder library only supports up to 8 encoders
 * @note       This check only applies when building for ESP32
 */
#define ELS_MAX_ENCODERS_ESP32 8
static_assert(ELS_TOTAL_ENCODERS <= ELS_MAX_ENCODERS_ESP32, \
    "Too many encoders for ESP32. ESP32 hardware encoder library only supports up to 8 encoders. " \
    "Current encoder count: " STRINGIFY(ELS_TOTAL_ENCODERS) \
    " (spindle: " STRINGIFY(defined(ELS_SPINDLE_DRIVEN) ? 0 : 1) \
    ", UI: " STRINGIFY(ARRAY_SIZE(ENCODER_DEFINITIONS)) ")")

    // Static instance for timer interrupt
    ESP32HIDHandler* ESP32HIDHandler::s_instance = nullptr;

ESP32HIDHandler::ESP32HIDHandler() : m_keycodeMillis(0), m_timer(nullptr), m_userHandler(nullptr) {
    s_instance = this;

    // Initialize button state
    m_buttonState.button = 0;
    m_buttonState.buttonState = BS_NONE;

    // Initialize encoders
    for (size_t i = 0; i < ELS_TOTAL_ENCODERS; ++i) {
        m_encoderPositions[i] = 0;
    }

    // Set up button array if enabled
#ifdef ELS_USE_BUTTON_ARRAY
    setupKeys();

    // Initialize timer for button hold detection
    m_timer = timerBegin(0, 80, true);
    timerAttachInterrupt(m_timer, &timerInterrupt, true);
    timerAlarmWrite(m_timer, 1000000, true); // 1 second
    timerStop(m_timer);
    timerAlarmEnable(m_timer);
#endif

    // Set up encoders from ENCODER_DEFINITIONS
    for (size_t i = 0; i < sizeof(ENCODER_DEFINITIONS) / sizeof(ENCODER_DEFINITIONS[0]); ++i) {
        const auto& def = ENCODER_DEFINITIONS[i];
        ESP32Encoder::useInternalWeakPullResistors = puType::none;
        m_encoders[i].attachSingleEdge(def.A, def.B);
        m_encoders[i].setFilter(1023);
        m_encoderPositions[i] = m_encoders[i].getCount();
    }

    // Set up spindle encoder if not driven
#ifndef ELS_SPINDLE_DRIVEN
    size_t spindleEncoderIndex = sizeof(ENCODER_DEFINITIONS) / sizeof(ENCODER_DEFINITIONS[0]);
    ESP32Encoder::useInternalWeakPullResistors = puType::none;
    m_encoders[spindleEncoderIndex].attachSingleEdge(ELS_SPINDLE_ENCODER_A, ELS_SPINDLE_ENCODER_B);
    m_encoders[spindleEncoderIndex].setFilter(1023);
    m_encoderPositions[spindleEncoderIndex] = m_encoders[spindleEncoderIndex].getCount();
#endif
}

void ESP32HIDHandler::setupKeys() {
#ifdef ELS_USE_BUTTON_ARRAY
    // Set pad H pins as input
    pinMode(ELS_PAD_H1, INPUT_PULLDOWN);
    pinMode(ELS_PAD_H2, INPUT_PULLDOWN);
    pinMode(ELS_PAD_H3, INPUT_PULLDOWN);

    // Set pad V pins as out, high
    pinMode(ELS_PAD_V1, OUTPUT);
    pinMode(ELS_PAD_V2, OUTPUT);
    pinMode(ELS_PAD_V3, OUTPUT);
    digitalWrite(ELS_PAD_V1, 1);
    digitalWrite(ELS_PAD_V2, 1);
    digitalWrite(ELS_PAD_V3, 1);
#endif
}

int ESP32HIDHandler::getCodeFromArray() {
#ifdef ELS_USE_BUTTON_ARRAY
    int a = digitalRead(ELS_PAD_H1) | (digitalRead(ELS_PAD_H2) << 1) | (digitalRead(ELS_PAD_H3) << 2);
    // Now, flip the input to V and set H high
    pinMode(ELS_PAD_V1, INPUT_PULLDOWN);
    pinMode(ELS_PAD_V2, INPUT_PULLDOWN);
    pinMode(ELS_PAD_V3, INPUT_PULLDOWN);
    pinMode(ELS_PAD_H1, OUTPUT);
    pinMode(ELS_PAD_H2, OUTPUT);
    pinMode(ELS_PAD_H3, OUTPUT);
    digitalWrite(ELS_PAD_H1, 1);
    digitalWrite(ELS_PAD_H2, 1);
    digitalWrite(ELS_PAD_H3, 1);
    // Now read the V states
    int b = digitalRead(ELS_PAD_V1) | (digitalRead(ELS_PAD_V2) << 1) | (digitalRead(ELS_PAD_V3) << 2);
    int code = (a == 0 || b == 0) ? 0 : a | b << 3;
    setupKeys();
    return code;
#else
    return 0;
#endif
}

void ESP32HIDHandler::handleTimer() {
    if (m_timer) {
        timerStop(m_timer);
    }

    int code = getCodeFromArray();
    if (m_buttonState.buttonState == ButtonState::BS_PRESSED && m_buttonState.button == code) {
        m_buttonState.buttonState = ButtonState::BS_HELD;
    } else {
        // if the same button isn't still pressed, then cancel the whole thing
        m_buttonState.buttonState = ButtonState::BS_NONE;
        m_buttonState.button = 0;
    }
}

ButtonInfo ESP32HIDHandler::consumeButton() {
    // Handle encoder updates
    for (size_t i = 0; i < sizeof(ENCODER_DEFINITIONS) / sizeof(ENCODER_DEFINITIONS[0]); ++i) {
        int64_t val = m_encoders[i].getCount();
        if (val != m_encoderPositions[i]) {
            updateEncoderPos(i, val - m_encoderPositions[i]);
        }
    }

    if (m_buttonState.buttonState == ButtonState::BS_PRESSED || m_buttonState.buttonState == ButtonState::BS_NONE) {
        return { 0, ButtonState::BS_NONE };
    }
    ButtonInfo ret = { m_buttonState.button, m_buttonState.buttonState };
    m_buttonState.buttonState = ButtonState::BS_NONE;
    m_buttonState.button = 0;
    return ret;
}

void ESP32HIDHandler::updateEncoderPos(int encoderIndex, int64_t pos) {
    if (encoderIndex >= sizeof(ENCODER_DEFINITIONS) / sizeof(ENCODER_DEFINITIONS[0])) {
        return; // Invalid encoder index
    }

    const auto& def = ENCODER_DEFINITIONS[encoderIndex];

    GlobalButtonLock lockState = GlobalState::getInstance()->getButtonLock();
    if (lockState == GlobalButtonLock::LOCKED) {
        m_encoderPositions[encoderIndex] += pos;
        return;
    }

    // Handle encoder actions based on direction
    if (pos > 0) {
        // Clockwise - trigger CW action
        if (m_userHandler) {
            m_userHandler->handleEncoderAction(def.CW_action, EncoderActionType::ROTATE_CW);
        }
    } else if (pos < 0) {
        // Counter-clockwise - trigger CCW action
        if (m_userHandler) {
            m_userHandler->handleEncoderAction(def.CCW_action, EncoderActionType::ROTATE_CCW);
        }
    }

    m_encoderPositions[encoderIndex] += pos;
}

void ESP32HIDHandler::setUserInteractionHandler(UserInteractionHandler* handler) {
    m_userHandler = handler;
}

void ESP32HIDHandler::processButtonEvents() {
    if (!m_userHandler) {
        return;
    }

    ButtonInfo buttonInfo = getButtonInfo();
    if (buttonInfo.button == 0) {
        return; // No button event
    }

    int row, col;
    if (getButtonCoordinates(buttonInfo.button, row, col)) {
        // Convert button state to ButtonActionType
        ButtonActionType actionType;
        switch (buttonInfo.buttonState) {
        case BS_CLICKED:
            actionType = ButtonActionType::PRESS;
            break;
        case BS_HELD:
            actionType = ButtonActionType::HOLD;
            break;
        case BS_RELEASED:
            actionType = ButtonActionType::RELEASE;
            break;
        case BS_DOUBLE_CLICKED:
            actionType = ButtonActionType::DOUBLE_PRESS;
            break;
        default:
            return; // Unknown button state
        }

        // Handle the button action through UserInteractionHandler
        m_userHandler->handleButtonAction(row, col, actionType);
    }
}

void IRAM_ATTR ESP32HIDHandler::timerInterrupt() {
    if (s_instance) {
        s_instance->handleTimer();
    }
}

void ESP32HIDHandler::handle() {
    unsigned long time = millis();

#ifdef ELS_USE_BUTTON_ARRAY
    if (time < m_keycodeMillis + 10) return; // debounce

    // Handle button array
    int code = getCodeFromArray();
    if (code == 0) {
        // Release
        if (m_timer) {
            timerStop(m_timer);
        }
        m_keycodeMillis = time;
        if (m_buttonState.buttonState == BS_PRESSED) {
            m_buttonState.buttonState = BS_CLICKED;
        }
    } else {
        m_buttonState.button = code;
        m_buttonState.buttonState = BS_PRESSED;
        m_keycodeMillis = time;
        if (m_timer) {
            timerWrite(m_timer, 0); // Reset timer counter
            timerStart(m_timer);
        }
    }

    // Process button events through UserInteractionHandler
    processButtonEvents();
#endif

    // Handle encoders
    for (size_t i = 0; i < sizeof(ENCODER_DEFINITIONS) / sizeof(ENCODER_DEFINITIONS[0]); ++i) {
        int64_t position = m_encoders[i].getCount();
        if (position != m_encoderPositions[i]) {
            updateEncoderPos(i, position - m_encoderPositions[i]);
        }
    }
}

ButtonInfo ESP32HIDHandler::getButtonInfo() {
    return consumeButton();
}

bool ESP32HIDHandler::getButtonCoordinates(int buttonCode, int& row, int& col) {
#ifdef ELS_USE_BUTTON_ARRAY
    // Convert button code to row/col coordinates
    // Button codes are generated as: (a | b << 3) where a and b are 3-bit values
    // a represents the H pins (0-2), b represents the V pins (0-2)
    if (buttonCode == 0) {
        return false;
    }

    int hBits = buttonCode & 0x07;  // Lower 3 bits
    int vBits = (buttonCode >> 3) & 0x07;  // Upper 3 bits

    // Find which H pin is active (0-2)
    for (int h = 0; h < 3; h++) {
        if (hBits & (1 << h)) {
            row = h;
            break;
        }
    }

    // Find which V pin is active (0-2)
    for (int v = 0; v < 3; v++) {
        if (vBits & (1 << v)) {
            col = v;
            break;
        }
    }

    return true;
#else
    return false;
#endif
}

#endif