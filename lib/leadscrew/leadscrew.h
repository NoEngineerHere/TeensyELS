#include <spindle.h>
#include <axis.h>
#include <algorithm>
#include <cstdint>
#include <cfloat>
#include "leadscrew_io.h"
#pragma once



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

enum class LeadscrewSpindleSyncPositionState { LEFT, RIGHT, UNSET };


class Leadscrew : public LinearAxis, public DerivedAxis, public DrivenAxis {
private:


  Spindle* m_spindle;
  LeadscrewIO* m_io;

  double m_expectedPosition = 0;

  // the ratio of how much the leadscrew moves per spindle rotation
  const int motorPulsePerRevolution;
  const float leadscrewPitch;
  // the number of pulses per revolution of the lead axis (spindle)
  const int encoderPPR;
  float m_ratio = 0;
  bool m_numericFault = false;
  bool m_syncInvalid = false;
  bool m_inhibit = true;
  bool m_faultInhibitLatched = false;
  bool m_initialized = false;
  bool m_initAttempted = false;
  bool m_ioFault = false;
  bool m_pulsePending = false;
  bool m_resetPending = true;
  bool m_positionValid = true;
  LeadscrewDirection m_pulseDirection = LeadscrewDirection::UNKNOWN;
  uint32_t m_lastPulseStartTimestamp = 0;
  uint32_t m_lowTimestamp = 0;
  uint32_t m_dirTimestamp = 0;

  // The current delay between pulses in microseconds
  const float initialPulseDelay;
  float m_currentPulseDelay;
  float m_leadscrewSpeed = 0;
  const float m_leadscrewAccel;
  LeadscrewDirection m_currentDirection = LeadscrewDirection::UNKNOWN;


  // we may want more sophisticated control over positions, but for now this is
  // fine
  LeadscrewStopState m_leftStopState = LeadscrewStopState::UNSET;
  int m_leftStopPosition = INT32_MIN;
  LeadscrewStopState m_rightStopState = LeadscrewStopState::UNSET;
  int m_rightStopPosition = INT32_MAX;
  LeadscrewSpindleSyncPositionState m_syncPositionState = LeadscrewSpindleSyncPositionState::UNSET;
  int m_spindleSyncPosition = 0;

  void resetProfile(uint32_t now);
  void latchIOFault(bool uncertain);
  int getStoppingDistanceInPulses();
  int getTargetSpeedDistanceInPulses();
  uint32_t jogMicros;

  bool phaseAtPosition(int position, int& phase) const;
  void setStopPositionUnlocked(LeadscrewStopPosition stop, int position);
  int boundedInt(double value);

public:
  Leadscrew(Spindle* spindle, LeadscrewIO* io,
    float leadscrewAccel, float initialPulseDelay, 
    int motorPulsePerRevolution,
    float leadscrewPitch, int encoderPPR);


  bool initializeOutputs();
  void setFaultInhibit(bool inhibit);
  bool isOutputIdle() const;
  void setStopPosition(LeadscrewStopPosition position);
  void setStopPosition(LeadscrewStopPosition position, int stopPosition);
  LeadscrewStopState getStopPositionState(LeadscrewStopPosition position);
  void unsetStopPosition(LeadscrewStopPosition position);
  int getStopPosition(LeadscrewStopPosition position);
  void setTargetPitchMM(float ratio) override;
  int getCurrentPosition() override;
  void setCurrentPosition(int position) override;
  void update() override;
  int getPositionError() override;
  LeadscrewDirection getCurrentDirection();
  float getEstimatedVelocityInMillimetersPerSecond() override;

};
