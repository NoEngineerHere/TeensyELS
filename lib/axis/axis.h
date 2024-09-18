#include <els_elapsedMillis.h>

#include <cstdint>

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

  // the elapsed time for the last full pulse duration
  uint32_t m_lastFullPulseDurationMicros;

 public:
  Axis() {
    m_lastPulseMicros = 0;
    m_lastFullPulseDurationMicros = 0;
    m_currentPosition = 0;
  }
  virtual int getCurrentPosition() { return m_currentPosition; }
  virtual void resetCurrentPosition() { m_currentPosition = 0; }
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

  /**
   * These will limit the position to one rotation within the position set
   * Left will be within position and position- ELS_SPINDLE_ENCODER_PPR
   * Right will be within position and position + ELS_SPINDLE_ENCODER_PPR
   */
  void setPositionLimit(SpindleLimitOption option, int position);
  void unsetPositionLimit(SpindleLimitOption option);
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
