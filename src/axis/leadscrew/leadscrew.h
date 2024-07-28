#include <elapsedMillis.h>  // Include the header file for elapsedMicros

#include "../axis.h"

#pragma once

class Leadscrew : public LinearAxis,
                  public DerivedAxis,
                  public DrivenAxis,
                  public MobileAxis {
 private:
  Axis* m_leadAxis;

  int m_currentPosition;

  // the ratio of how much the leadscrew moves per spindle rotation
  float m_ratio;

  // the timestamp of the last pulse
  elapsedMicros m_lastPulseMicros;

  // The current delay between pulses in microseconds
  int m_currentPulseDelay;

  float m_accumulator;

  int m_cycleModulo;

  /**
   * This gets the "unit" of the accumulator, i.e the amount the accumulator
   * increased by when the leadscrew position increases by 1
   */
  float getAccumulatorUnit();
  bool sendPulse();
  // int getStoppingDistanceInPulses();

 public:
  Leadscrew(Axis* leadAxis);
  int getCurrentPosition();
  void resetCurrentPosition();
  void setRatio(float ratio);
  float getRatio();
  int getExpectedPosition();
  void setCurrentPosition(int position);
  void incrementCurrentPosition(int amount);
  void update();
  int getPositionError();
  float getEstimatedVelocityInPulsesPerSecond();
  float getEstimatedVelocityInMillimetersPerSecond();
};
