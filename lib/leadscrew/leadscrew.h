#include <axis.h>
#include <els_elapsedMillis.h>
#pragma once

class Leadscrew : public LinearAxis, public DerivedAxis, public DrivenAxis {
 private:
  Axis* m_leadAxis;

  // the ratio of how much the leadscrew moves per spindle rotation
  float m_ratio;

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
  float getEstimatedVelocityInMillimetersPerSecond();
};
