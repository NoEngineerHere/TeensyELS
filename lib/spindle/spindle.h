#include "config.h"

#if ELS_BOARD == ELS_BOARD_ESP32
#include <ESP32Encoder.h>
#elif ELS_BOARD == ELS_BOARD_TEENSY
#include <Encoder.h>
#endif
#include <axis.h>

#pragma once

class Spindle : public RotationalAxis {
private:
    // the unconsumed position is the position that has been read from the encoder
    // but hasn't been used to update the current position of any driven axes
    int m_unconsumedPosition;

#ifndef ELS_SPINDLE_DRIVEN
#if ELS_BOARD == ELS_BOARD_ESP32
    ESP32Encoder  m_encoder;
#elif ELS_BOARD == ELS_BOARD_TEENSY
    Encoder  m_encoder;
#endif
#endif

public:
#ifndef ELS_SPINDLE_DRIVEN
    Spindle(int pinA, int pinB);
#endif

    void update();
    void setCurrentPosition(int position);
    void incrementCurrentPosition(int amount);
    /**
     * This will return the unconsumed position and reset it to 0
     * used for updating the expected position of any driven axes
     */
    int consumePosition();
    float getEstimatedVelocityInRPM();
    float getEstimatedVelocityInPPS();
};