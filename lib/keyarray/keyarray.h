#pragma once

#ifdef ELS_USE_BUTTON_ARRAY
#ifndef KEYARRAY_H
#define KEYARRAY_H

#include <Arduino.h>
#include <leadscrew.h>
#include <spindle.h>
#include <ESP32Encoder.h>

void buttonInterrupt();
void IRAM_ATTR timerInterrupt();

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

class KeyArray {
private:
    Leadscrew* m_leadscrew;
    volatile ButtonInfo buttonState;
    volatile unsigned long keycodeMillis;
    hw_timer_t* Timer0_Cfg;
    void setupKeys();
    int getCodeFromArray();
    void updateEncoderPos(int64_t pos);
#ifdef ELS_UI_ENCODER
    ESP32Encoder m_encoder;
    int64_t encoderPos;
#endif
public:
    KeyArray();
    void initPad();
    void handle();
    void handleTimer();
    ButtonInfo consumeButton();
    KeyArray(Leadscrew* leadscrew);
};

extern KeyArray keyArray;
#endif
#endif 