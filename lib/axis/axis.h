#include <els_elapsedMillis.h>

#pragma once

/**
 * A basic axis - should have a position
 * This can either be driven externally or by the application
 */
class Axis {
 protected:
  volatile int m_currentPosition;

  // the timestamp of the last pulse
  elapsedMicros m_lastPulseMicros;

 public:
  virtual int getCurrentPosition() { return m_currentPosition; }
  virtual void resetCurrentPosition() { m_currentPosition = 0; }
  virtual float getEstimatedVelocityInPulsesPerSecond() {
    // ideally we would average out over the last few pulses
    // but that would take effort
    if (m_lastPulseMicros == 0) {
      return 0;
    }

    return (float)(1000000 / (micros() - m_lastPulseMicros));
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
  virtual void setRatio(float ratio) = 0;
  virtual float getRatio() = 0;
};

/**
 * An axis that is driven by the application
 * Example: The leadscrew is a driven axis since it has a motor we control
 * attached
 */
class DrivenAxis {
 public:
  virtual int getExpectedPosition() = 0;
  virtual void update() = 0;
  virtual int getPositionError() = 0;
};