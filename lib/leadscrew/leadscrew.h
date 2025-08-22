#ifdef PIO_UNIT_TESTING
#include "TestSpindle.h"
#else
#include <spindle.h>
#endif
#include <axis.h>
#include "../interfaces/system_interfaces.h"
#ifdef PIO_UNIT_TESTING
#include "../../test/arduino_test_mock.h"
#else
#include <Arduino.h>
#endif
#include "leadscrew_io.h"
#include <config.h>
#pragma once

struct LeadscrewConfig {
  float accel;
  float initialPulseDelay;
  int motorPulsePerRevolution;
  float pitch;
  int encoderPPR;
  
  // Default configuration using config.h values
  LeadscrewConfig(float accel = 0, float initialPulseDelay = LEADSCREW_INITIAL_PULSE_DELAY_US,
                  int motorPPR = ELS_LEADSCREW_STEPPER_PPR * ELS_GEARBOX_RATIO,
                  float pitch = ELS_LEADSCREW_PITCH_MM, int encoderPPR = ELS_SPINDLE_ENCODER_PPR)
    : accel(accel), initialPulseDelay(initialPulseDelay), motorPulsePerRevolution(motorPPR),
      pitch(pitch), encoderPPR(encoderPPR) {}
};


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

enum class LeadscrewSpindleSyncPositionState { LEFT, RIGHT, UNSET };


class Leadscrew : public LinearAxis, public DerivedAxis, public DrivenAxis, public ILeadscrew {
public:
    // Override getCurrentPosition to satisfy ILeadscrew interface
    int getCurrentPosition() override { return Axis::getCurrentPosition(); }
private:

#ifdef ELS_USE_RMT
  rmt_data_t rmt_data[24];
  rmt_obj_t *rmtObj;
#endif

  Spindle* m_spindle;
  LeadscrewIO* m_io;

  float m_expectedPosition;

  // the ratio of how much the leadscrew moves per spindle rotation
  const int motorPulsePerRevolution;
  const float leadscrewPitch;
  // the number of pulses per revolution of the lead axis (spindle)
  const int encoderPPR;
  float m_ratio;

  // The current delay between pulses in microseconds
  const float initialPulseDelay;
  float m_currentPulseDelay;
  float m_leadscrewSpeed;
  const float m_leadscrewAccel;
  LeadscrewDirection m_currentDirection;

  // Stop position management
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
  bool sendPulse();
  int getStoppingDistanceInPulses();
  int getTargetSpeedDistanceInPulses();
  uint64_t jogMicros;

  int debugPulseCount;
  bool initPos;

public:
  // Modern constructor using configuration struct
  Leadscrew(Spindle* spindle, LeadscrewIO* io, const LeadscrewConfig& config);
  
  // Legacy constructor for backward compatibility
  Leadscrew(Spindle* spindle, LeadscrewIO* io,
    float leadscrewAccel, float initialPulseDelay, 
    int motorPulsePerRevolution,
    float leadscrewPitch, int encoderPPR);
  #ifdef ELS_USE_RMT
  void setRMT(rmt_obj_t *rmtObj){
    this->rmtObj = rmtObj;
    rmt_data->duration0 = 8;
    rmt_data->level0 = 1;
    rmt_data->duration1 = 8;
    rmt_data->level1 = 0;
  
  }
  #endif


  void setStopPosition(LeadscrewStopPosition position);
  void setStopPosition(LeadscrewStopPosition position, int stopPosition);
  LeadscrewStopState getStopPositionState(LeadscrewStopPosition position);
  void unsetStopPosition(LeadscrewStopPosition position);
  int getStopPosition(LeadscrewStopPosition position);
  void setTargetPitchMM(float ratio);
  void setCurrentPosition(int position);
  void update();
  int getPositionError();
  LeadscrewDirection getCurrentDirection();
  float getEstimatedVelocityInMillimetersPerSecond();

};
