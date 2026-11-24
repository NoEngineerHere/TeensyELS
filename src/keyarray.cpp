#include <els/hardware_config.h>
#if ELS_INPUT_LAYOUT == ELS_INPUT_LEGACY_MATRIX
#include <keyarray.h>




void KeyArray::setupKeys() {

    // Set pad H pins as input
    pinMode(hardware.matrix.rows[0], INPUT_PULLDOWN);
    pinMode(hardware.matrix.rows[1], INPUT_PULLDOWN);
    pinMode(hardware.matrix.rows[2], INPUT_PULLDOWN);

    // Set pad V pins as out, high
    pinMode(hardware.matrix.columns[0], OUTPUT);
    pinMode(hardware.matrix.columns[1], OUTPUT);
    pinMode(hardware.matrix.columns[2], OUTPUT);
    digitalWrite(hardware.matrix.columns[0], 1);
    digitalWrite(hardware.matrix.columns[1], 1);
    digitalWrite(hardware.matrix.columns[2], 1);

    attachInterrupt(digitalPinToInterrupt(hardware.matrix.rows[0]), buttonInterrupt, CHANGE);
    attachInterrupt(digitalPinToInterrupt(hardware.matrix.rows[1]), buttonInterrupt, CHANGE);
    attachInterrupt(digitalPinToInterrupt(hardware.matrix.rows[2]), buttonInterrupt, CHANGE);

}
bool KeyArray::initPad() {
    if constexpr (hardware.encoder.a >= 0) {
        ESP32Encoder::useInternalWeakPullResistors = hardware.encoder.externalPulls ? puType::none : puType::up;
        m_encoder.attachSingleEdge(hardware.encoder.a, hardware.encoder.b);
        if (!m_encoder.isAttached()) return false;
        m_encoder.setFilter(1023);
        encoderPos = m_encoder.getCount();
    }
    Timer0_Cfg = timerBegin(0, 80, true);
    timerAttachInterrupt(Timer0_Cfg, &timerInterrupt, true);
    timerAlarmWrite(Timer0_Cfg, 1000000, true);
    timerStop(Timer0_Cfg);
    timerAlarmEnable(Timer0_Cfg);
    setupKeys();
    return true;
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
    if (buttonState.buttonState == ButtonState::BS_PRESSED || buttonState.buttonState == ButtonState::BS_NONE) {
        return { 0, ButtonState::BS_NONE };
    }
    ButtonInfo ret = { buttonState.button, buttonState.buttonState };
    buttonState.buttonState = ButtonState::BS_NONE;
    buttonState.button = 0;
    return ret;
}

#if defined(ESP32)
int64_t KeyArray::consumeEncoderDelta() {
    const int64_t count = m_encoder.getCount();
    const int64_t delta = count - encoderPos;
    encoderPos = count;
    return delta;
}
#endif

int KeyArray::getCodeFromArray() {
    int a = digitalRead(hardware.matrix.rows[0]) | (digitalRead(hardware.matrix.rows[1]) << 1) | (digitalRead(hardware.matrix.rows[2]) << 2);
    // Now, flip the input to V and set H high
    pinMode(hardware.matrix.columns[0], INPUT_PULLDOWN);
    pinMode(hardware.matrix.columns[1], INPUT_PULLDOWN);
    pinMode(hardware.matrix.columns[2], INPUT_PULLDOWN);
    pinMode(hardware.matrix.rows[0], OUTPUT);
    pinMode(hardware.matrix.rows[1], OUTPUT);
    pinMode(hardware.matrix.rows[2], OUTPUT);
    digitalWrite(hardware.matrix.rows[0], 1);
    digitalWrite(hardware.matrix.rows[1], 1);
    digitalWrite(hardware.matrix.rows[2], 1);
    // Now read the V states
    int b = digitalRead(hardware.matrix.columns[0]) | (digitalRead(hardware.matrix.columns[1]) << 1) | (digitalRead(hardware.matrix.columns[2]) << 2);
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

