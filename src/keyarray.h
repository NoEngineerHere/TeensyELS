#include <els/hardware_config.h>
#if ELS_INPUT_LAYOUT == ELS_INPUT_LEGACY_MATRIX
#ifndef KEYARRAY_H
#define KEYARRAY_H

#include <Arduino.h>
#if defined(ESP32)
#include <ESP32Encoder.h>
#endif

void buttonInterrupt();
void IRAM_ATTR timerInterrupt();

enum ButtonState { BS_NONE = 0, BS_PRESSED = 1, BS_CLICKED = 2, BS_HELD = 3, BS_RELEASED = 4, BS_DOUBLE_CLICKED = 5 };

typedef struct buttonInfo {
    int button;
    int buttonState;
} ButtonInfo;

class KeyArray {
private:
    volatile ButtonInfo buttonState;
    volatile unsigned long keycodeMillis;
    hw_timer_t* Timer0_Cfg;
    void setupKeys();
    int getCodeFromArray();
#if defined(ESP32)
    ESP32Encoder m_encoder;
    int64_t encoderPos;
#endif
public:
    bool initPad();
    void handle();
    void handleTimer();
    ButtonInfo consumeButton();
#if defined(ESP32)
    int64_t consumeEncoderDelta();
#endif
};

extern KeyArray keyArray;
#endif
#endif