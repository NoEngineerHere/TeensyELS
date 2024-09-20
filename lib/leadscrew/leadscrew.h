#include <spindle.h>
#include <els_elapsedMillis.h>

#include "leadscrew_io.h"
#pragma once

enum LeadscrewStopState { SET, UNSET };
enum LeadscrewDirection { LEFT = -1, RIGHT = 1, UNKNOWN = 0 };

class Leadscrew : public LinearAxis, public DerivedAxis, public DrivenAxis {
 private:
  Spindle* m_spindle;
  LeadscrewIO* m_io;

  float m_expectedPosition;

  // the ratio of how much the leadscrew moves per spindle rotation
  const int motorPulsePerRevolution;
  const float leadscrewPitch;
  // the number of pulses per revolution of the lead axis (spindle)
  const int leadAxisPPR;
  float m_ratio;

  // The current delay between pulses in microseconds
  const float initialPulseDelay;
  const float pulseDelayIncrement;
  float m_currentPulseDelay;
  LeadscrewDirection m_currentDirection;

  float m_accumulator;

  // we may want more sophisticated control over positions, but for now this is
  // fine
  LeadscrewStopState m_leftStopState;
  int m_leftStopPosition;
  LeadscrewStopState m_rightStopState;
  int m_rightStopPosition;

  /**
   * This gets the "unit" of the accumulator, i.e the amount the accumulator
   * increased by when the leadscrew position increases by 1
   */
  float getAccumulatorUnit();
  bool sendPulse();
  // int getStoppingDistanceInPulses();

 public:
  Leadscrew(Spindle* spindle, LeadscrewIO* io, float initialPulseDelay,
            float pulseDelayIncrement, int motorPulsePerRevolution,
            float leadscrewPitch, int leadAxisPPR);
  int getCurrentPosition();
  void resetCurrentPosition();

  enum StopPosition { LEFT, RIGHT };
  void setStopPosition(StopPosition position, int stopPosition);
  LeadscrewStopState getStopPositionState(StopPosition position);
  void unsetStopPosition(StopPosition position);
  int getStopPosition(StopPosition position);
  void setRatio(float ratio);
  float getRatio();
  int getExpectedPosition();
  void setCurrentPosition(int position);
  void incrementCurrentPosition(int amount);
  void update();
  int getPositionError();
  LeadscrewDirection getCurrentDirection();
  float getEstimatedVelocityInMillimetersPerSecond();

  void printState();
};
