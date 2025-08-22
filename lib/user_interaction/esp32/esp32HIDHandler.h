#pragma once

#include <config.h>

#if ELS_BOARD == ELS_BOARD_ESP32
#include "HIDHandler.h"
#include <Arduino.h>
#include <ESP32Encoder.h>
#include <UserInteractionHandler.h>

/**
 * @brief      Calculate the total number of encoders used by the system
 * @note       This includes:
 *              - 1 spindle encoder (when ELS_SPINDLE_DRIVEN is not defined)
 *              - UI encoders from ENCODER_DEFINITIONS array
 * @note       Current count: 3 total (2 UI + 1 spindle when not driven)
 */
#define ELS_TOTAL_ENCODERS \
  (ARRAY_SIZE(ENCODER_DEFINITIONS) + \
   (defined(ELS_SPINDLE_DRIVEN) ? 0 : 1))

 // Button state enum similar to keyarray.h
enum ButtonState {
    BS_NONE = 0,
    BS_PRESSED = 1,
    BS_CLICKED = 2,
    BS_HELD = 3,
    BS_RELEASED = 4,
    BS_DOUBLE_CLICKED = 5
};

typedef struct buttonInfo {
    int button;
    int buttonState;
} ButtonInfo;

class ESP32HIDHandler : public HIDHandler {
public:
    ESP32HIDHandler();
    void handle() override;

    // Public method to get button information (similar to KeyArray::consumeButton)
    ButtonInfo getButtonInfo();

    // Convert button code to row/col coordinates for UserInteractionHandler
    bool getButtonCoordinates(int buttonCode, int& row, int& col);

    // Set the UserInteractionHandler for handling encoder actions
    void setUserInteractionHandler(UserInteractionHandler* handler);

    // Process button events through UserInteractionHandler
    void processButtonEvents();

private:
    ESP32Encoder m_encoders[ELS_TOTAL_ENCODERS];
    int64_t m_encoderPositions[ELS_TOTAL_ENCODERS];

    // Button array handling (similar to KeyArray)
    volatile ButtonInfo m_buttonState;
    volatile unsigned long m_keycodeMillis;
    hw_timer_t* m_timer;

    // User interaction handler for processing actions
    UserInteractionHandler* m_userHandler;

    // Button array methods
    void setupKeys();
    int getCodeFromArray();
    void handleTimer();
    ButtonInfo consumeButton();

    // Encoder handling
    void updateEncoderPos(int encoderIndex, int64_t pos);

    // Timer interrupt handler
    static void IRAM_ATTR timerInterrupt();
    static ESP32HIDHandler* s_instance;

};

#endif