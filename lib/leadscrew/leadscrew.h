#include <spindle.h>
#include <els_elapsedMillis.h>

#include "leadscrew_io.h"
#pragma once

// only run for unit tests
#if PIO_UNIT_TESTING
#undef ELS_INVERT_DIRECTION
#endif

/**
 * The state of the leadscrew stop position for either the left or right stop
 */
enum class LeadscrewStopState { SET, UNSET };
enum class LeadscrewStopPosition { LEFT, RIGHT };
/**
 * The current direction of the leadscrew
 * We set numbers to use later when actually moving the position
 */
enum class LeadscrewDirection { LEFT = -1, RIGHT = 1, UNKNOWN = 0 };

/**
 * The state of the spindle sync position
 * The spindle sync position is a known position of the spindle that syncs with the current thread
 * Since the spindle is a "rotational" axis and the leadscrew is a "linear" axis, we need to know
 * an anchor point of where the spindle and the leadscrew are both in sync.
 * 
 * We reuse the endstop states for this, since they are similar in nature and keep the first one that is set
 */

enum class LeadscrewSpindleSyncPositionState {LEFT, RIGHT, UNSET};


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

  LeadscrewSpindleSyncPositionState m_syncPositionState;
  int m_spindleSyncPosition;

  /**
   * This gets the "unit" of the accumulator, i.e the amount the accumulator
   * increased by when the leadscrew position increases by 1
   */
  float getAccumulatorUnit();
  bool sendPulse();
  int getStoppingDistanceInPulses();
  void setStopPosition(LeadscrewStopPosition position, int stopPosition);

 public:
  Leadscrew(Spindle* spindle, LeadscrewIO* io, float initialPulseDelay,
            float pulseDelayIncrement, int motorPulsePerRevolution,
            float leadscrewPitch, int leadAxisPPR);
  int getCurrentPosition();
  void resetCurrentPosition();


  void setStopPosition(LeadscrewStopPosition position);
  LeadscrewStopState getStopPositionState(LeadscrewStopPosition position);
  void unsetStopPosition(LeadscrewStopPosition position);
  int getStopPosition(LeadscrewStopPosition position);
  void setRatio(float ratio);
  float getRatio();
  float getExpectedPosition();
  void setExpectedPosition(float position);
  void setCurrentPosition(int position);
  void incrementCurrentPosition(int amount);
  void update();
  int getPositionError();
  LeadscrewDirection getCurrentDirection();
  float getEstimatedVelocityInMillimetersPerSecond();

  void printState();
};
