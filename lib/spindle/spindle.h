#ifndef PIO_UNIT_TESTING
#if defined(ESP32)
#include <ESP32Encoder.h>
#elif defined(CORE_TEENSY)
#include <Encoder.h>
#endif
#endif
#include <axis.h>
#include "../interfaces/system_interfaces.h"

#pragma once

#ifndef PIO_UNIT_TESTING
class Spindle : public RotationalAxis, public ISpindle {
public:
    // Explicitly use Axis::getCurrentPosition to resolve ambiguity
    using Axis::getCurrentPosition;
private:
    // the unconsumed position is the position that has been read from the encoder
    // but hasn't been used to update the current position of any driven axes
    int m_unconsumedPosition;

#ifndef ELS_SPINDLE_DRIVEN
#if defined(ESP32)
    ESP32Encoder  m_encoder;
#elif defined(CORE_TEENSY)
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
#endif // PIO_UNIT_TESTING