
#include <cstdint>
#ifdef PIO_UNIT_TESTING
#include "../../test/arduino_test_mock.h"
#else
#include <Arduino.h>
#endif

#pragma once

/**
 * A basic axis - should have a position
 * This can either be driven externally or by the application
 */
class Axis {
protected:
  int m_currentPosition;

  // the timestamp of the last pulse
  int64_t m_lastPulseTimestamp;
  int64_t m_lastRevTimestamp;
  int64_t m_lastRevPosition;
  int64_t m_lastRevSize;
  int64_t m_lastRevMicros;

  // the elapsed time for the last full pulse duration
  uint32_t m_lastFullPulseDurationMicros;

public:
  Axis() {
    m_lastPulseTimestamp = micros();
    m_lastFullPulseDurationMicros = 0;
    m_currentPosition = 0;
  }
  virtual int getCurrentPosition() { return m_currentPosition; }
  virtual uint32_t getEstimatedVelocityInPulsesPerSecond() {
    // ensure that we're not in some ridiculous state where the spindle has
    // stopped for a long time
    if (m_lastFullPulseDurationMicros == 0 ||
      m_lastFullPulseDurationMicros > 1000) {
      return 0;
    }

    return 1000000 / m_lastFullPulseDurationMicros;
  }

public:
  virtual void setCurrentPosition(int position) {
    m_currentPosition = position;
  }
  virtual void incrementCurrentPosition(int amount) {
    m_currentPosition += amount;
  }
};

class RotationalAxis : public Axis {
public:
  virtual float getEstimatedVelocityInRPM() = 0;
};

class LinearAxis : public Axis {
public:
  virtual float getEstimatedVelocityInMillimetersPerSecond() = 0;
};

/**
 * An axis that is derived from the position of another axis
 * Example: the leadscrew is derived from the position of the spindle
 */
class DerivedAxis {
public:
  virtual void setTargetPitchMM(float ratio) = 0;
};

/**
 * An axis that is driven by the application
 * Example: The leadscrew is a driven axis since it has a motor we control
 * attached
 * positioning is expected to be a float due to <1 ratios between lead axis
 */
class DrivenAxis {
public:
  virtual void update() = 0;
  virtual int getPositionError() = 0;
};
