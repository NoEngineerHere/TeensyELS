#pragma once
#include <config.h>
#if defined(ESP32)
#include <ESP32Encoder.h>
#elif defined(CORE_TEENSY)
#include <Encoder.h>
#endif
#include <axis.h>


class Spindle : public RotationalAxis {
private:
    friend class Leadscrew; // Coherent anchor capture while the shared guard is held.
    int positionUnlocked() const { return m_currentPosition; }
    // the unconsumed position is the position that has been read from the encoder
    // but hasn't been used to update the current position of any driven axes
    int m_unconsumedPosition = 0;
    bool m_motionFault = false;
    bool m_idle = false;
    struct VelocitySnapshot {
      uint32_t timestamp;
      uint32_t duration;
      int64_t counts;
      bool valid;
    };
    VelocitySnapshot m_velocity{0, 0, 0, false};
    uint32_t m_sampleTimestamp = elsMicros();
    int64_t m_sampleCounts = 0;
    void incrementUnlocked(int amount, uint32_t now);
    void latchMotionFault();

#ifndef ELS_SPINDLE_DRIVEN
#if defined(ESP32)
    ESP32Encoder  m_encoder;
    int64_t m_lastEncoderCount = 0;
#elif defined(CORE_TEENSY)
    Encoder m_encoder;
#endif
#endif

public:
#if defined(PIO_UNIT_TESTING) || defined(ELS_SPINDLE_DRIVEN)
    Spindle();
#else
    Spindle(int pinA, int pinB);
#endif

    void update();
    bool hasMotionFault() const;
    int getCurrentPosition() override;
    void setCurrentPosition(int position) override;
    void incrementCurrentPosition(int amount) override;
    /**
     * This will return the unconsumed position and reset it to 0
     * used for updating the expected position of any driven axes
     */
    int consumePosition();
    float getEstimatedVelocityInRPM() override;
    float getEstimatedVelocityInPPS();
};