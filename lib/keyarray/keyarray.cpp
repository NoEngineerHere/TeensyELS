#include <config.h>

// button array is only supported on ESP32 right now
#if ELS_BOARD == ELS_BOARD_ESP32
#ifdef ELS_USE_BUTTON_ARRAY
#include <keyarray.h>
#include <globalstate.h>



KeyArray::KeyArray(Leadscrew* leadscrew) : m_leadscrew(leadscrew) {
#ifdef ELS_UI_ENCODER
    ESP32Encoder::useInternalWeakPullResistors = puType::none;
    m_encoder.attachSingleEdge(ELS_UI_ENCODER_A, ELS_UI_ENCODER_B);
    m_encoder.setFilter(1023);
    encoderPos = m_encoder.getCount();
#endif    
}

void KeyArray::setupKeys() {

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

    attachInterrupt(digitalPinToInterrupt(ELS_PAD_H1), buttonInterrupt, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ELS_PAD_H2), buttonInterrupt, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ELS_PAD_H3), buttonInterrupt, CHANGE);

}

void KeyArray::initPad() {

    Timer0_Cfg = timerBegin(0, 80, true);
    timerAttachInterrupt(Timer0_Cfg, &timerInterrupt, true);
    timerAlarmWrite(Timer0_Cfg, 1000000, true);
    timerStop(Timer0_Cfg);
    timerAlarmEnable(Timer0_Cfg);
    setupKeys();
}

void KeyArray::handleTimer() {
    timerStop(Timer0_Cfg);
    //DEBUG_F("Held");
    int code = getCodeFromArray();
    if (buttonState.buttonState == ButtonState::BS_PRESSED && buttonState.button == code) {
        buttonState.buttonState = ButtonState::BS_HELD;
    } else {
        // if the same button isnt' still pressed, then cancel the whole thing. 
        buttonState.buttonState = ButtonState::BS_NONE;
        buttonState.button = 0;
    }
}

ButtonInfo KeyArray::consumeButton() {
#ifdef ELS_UI_ENCODER
    int64_t val = m_encoder.getCount();
    if (val != encoderPos) {
        updateEncoderPos(val - encoderPos);
    }
#endif
    if (buttonState.buttonState == ButtonState::BS_PRESSED || buttonState.buttonState == ButtonState::BS_NONE) {
        return { 0, ButtonState::BS_NONE };
    }
    ButtonInfo ret = { buttonState.button, buttonState.buttonState };
    buttonState.buttonState = ButtonState::BS_NONE;
    buttonState.button = 0;
    return ret;
}

void KeyArray::updateEncoderPos(int64_t pos) {
#ifdef ELS_UI_ENCODER

    GlobalButtonLock lockState = GlobalState::getInstance()->getButtonLock();
    if (lockState == GlobalButtonLock::LK_LOCKED) {
        //DEBUG_F("Locked, ingoring rat inc");
        encoderPos += pos;
        return;
    }
    int64_t p = pos;
    if (pos > 0) {
        while (p-- > 0) {
            GlobalState::getInstance()->nextFeedPitch();
            m_leadscrew->setTargetPitchMM(GlobalState::getInstance()->getCurrentFeedPitch());
        }
    } else {
        while (p++ < 0) {
            GlobalState::getInstance()->prevFeedPitch();
            m_leadscrew->setTargetPitchMM(GlobalState::getInstance()->getCurrentFeedPitch());
        }
    }
    encoderPos += pos;
#endif
}

int KeyArray::getCodeFromArray() {
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

}

void KeyArray::handle() {
    unsigned long time = millis();
    if (time < keycodeMillis + 10)return; // debounce
    // First read the H states
    int code = getCodeFromArray();
    if (code == 0) {
        // Release
        timerStop(Timer0_Cfg);
        keycodeMillis = time;
        if (buttonState.buttonState == BS_PRESSED) {
            buttonState.buttonState = BS_CLICKED;
        }
    } else {
        //DEBUG_F("Pressed %d\n", code);
        buttonState.button = code;
        buttonState.buttonState = BS_PRESSED;
        keycodeMillis = time;
        timerRestart(Timer0_Cfg);
        timerStart(Timer0_Cfg);
    }
}


void buttonInterrupt() {
    keyArray.handle();
}

void IRAM_ATTR timerInterrupt() {
    keyArray.handleTimer();
}

#endif

#endif 