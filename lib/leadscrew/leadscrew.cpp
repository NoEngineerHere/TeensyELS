#include "leadscrew.h"

#include <config.h>
#include <globalstate.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include "leadscrew_io.h"
#include "../di/dependency_container.h"
using namespace std;

/**
 * Leadscrew controller implementing position tracking, motor control, and safety features.
 * 
 * NOTE: This class currently handles multiple responsibilities and should be refactored
 * into separate components for position control, motor control, and safety management.
 */

// Modern constructor using configuration struct
Leadscrew::Leadscrew(Spindle* spindle, LeadscrewIO* io, const LeadscrewConfig& config)
  : Leadscrew(spindle, io, config.accel, config.initialPulseDelay,
              config.motorPulsePerRevolution, config.pitch, config.encoderPPR) {
}

// DI-aware constructor
Leadscrew::Leadscrew(DependencyContainer* container, const LeadscrewConfig& config)
  : Leadscrew(static_cast<Spindle*>(container->resolve<ISpindle>()), 
              container->resolve<LeadscrewIO>(), config) {
}

// Legacy constructor for backward compatibility
Leadscrew::Leadscrew(Spindle* spindle, LeadscrewIO* io,
  float leadscrewAccel, float initialPulseDelay,
  int motorPulsePerRevolution,
  float leadscrewPitch, int encoderPPR)
  : motorPulsePerRevolution(motorPulsePerRevolution),
  leadscrewPitch(leadscrewPitch),
  encoderPPR(encoderPPR),
  m_io(io),
  m_spindle(spindle),
  initPos(false),
  m_currentDirection(LeadscrewDirection::UNKNOWN),
  m_leftStopState(LeadscrewStopState::UNSET),
  m_rightStopState(LeadscrewStopState::UNSET),
  m_leftStopPosition(INT32_MIN),
  m_rightStopPosition(INT32_MAX),
  m_leadscrewAccel(leadscrewAccel),
  m_leadscrewSpeed(0),
  initialPulseDelay(initialPulseDelay),
  m_currentPulseDelay(initialPulseDelay) {
  setTargetPitchMM(GlobalState::getInstance()->getCurrentFeedPitch());
  m_lastPulseTimestamp = micros();
  m_lastFullPulseDurationMicros = 0;
  m_expectedPosition = 0;
  m_currentPosition = 0;
}

void Leadscrew::setTargetPitchMM(float pitch) {
  // Calculate the ratio one, when the pitch is set. No need to calculate this every cycle
  m_ratio = (pitch * (float)motorPulsePerRevolution) / (leadscrewPitch * (float)encoderPPR);
}


void Leadscrew::unsetStopPosition(LeadscrewStopPosition position) {
  switch (position) {
  case LeadscrewStopPosition::LEFT:
    m_leftStopState = LeadscrewStopState::UNSET;
    m_leftStopPosition = INT32_MIN;
    if (m_syncPositionState == LeadscrewSpindleSyncPositionState::LEFT) {
      m_syncPositionState = LeadscrewSpindleSyncPositionState::UNSET;
      // extrapolate the sync position to the other endstop if set
      if (m_rightStopState == LeadscrewStopState::SET) {
        m_spindleSyncPosition = ((int)(static_cast<int>(m_syncPositionState) + (m_rightStopPosition - m_leftStopPosition) * m_ratio)) % encoderPPR;
        m_syncPositionState = LeadscrewSpindleSyncPositionState::RIGHT;
      }
    }
    break;
  case LeadscrewStopPosition::RIGHT:
    m_rightStopState = LeadscrewStopState::UNSET;
    m_rightStopPosition = INT32_MAX;
    if (m_syncPositionState == LeadscrewSpindleSyncPositionState::RIGHT) {
      m_spindleSyncPosition = ((int)(static_cast<int>(m_syncPositionState) + (m_rightStopPosition - m_leftStopPosition) * m_ratio)) % encoderPPR;
      m_syncPositionState = LeadscrewSpindleSyncPositionState::LEFT;
    }
    break;
  }
}

/**
 * Public facing api: only allows setting the stop position to the current position of the tool
 */
void Leadscrew::setStopPosition(LeadscrewStopPosition position) {
  setStopPosition(position, m_currentPosition);
}

void Leadscrew::setStopPosition(LeadscrewStopPosition position, int stopPosition) {
  switch (position) {
  case LeadscrewStopPosition::LEFT:
    m_leftStopPosition = stopPosition;
    m_leftStopState = LeadscrewStopState::SET;
    if (m_syncPositionState == LeadscrewSpindleSyncPositionState::UNSET && stopPosition == m_currentPosition) {
      m_spindleSyncPosition = m_spindle->getCurrentPosition();
      m_syncPositionState = LeadscrewSpindleSyncPositionState::LEFT;
      GlobalState::getInstance()->setThreadSyncState(GlobalThreadSyncState::SYNC);
    }
    break;
  case LeadscrewStopPosition::RIGHT:
    m_rightStopPosition = stopPosition;
    m_rightStopState = LeadscrewStopState::SET;
    if (m_syncPositionState == LeadscrewSpindleSyncPositionState::UNSET && stopPosition == m_currentPosition) {
      m_spindleSyncPosition = m_spindle->getCurrentPosition();
      m_syncPositionState = LeadscrewSpindleSyncPositionState::RIGHT;
      GlobalState::getInstance()->setThreadSyncState(GlobalThreadSyncState::SYNC);
    }
    break;
  }
}

LeadscrewStopState Leadscrew::getStopPositionState(LeadscrewStopPosition position) {
  switch (position) {
  case LeadscrewStopPosition::LEFT:
    return m_leftStopState;
  case LeadscrewStopPosition::RIGHT:
    return m_rightStopState;
  }
  return m_rightStopState;
}

int Leadscrew::getStopPosition(LeadscrewStopPosition position) {
  // todo better default values when unset
  switch (position) {
  case LeadscrewStopPosition::LEFT:
    if (m_leftStopState == LeadscrewStopState::SET) {
      return m_leftStopPosition;
    }
    return INT32_MIN;
  case LeadscrewStopPosition::RIGHT:
    if (m_rightStopState == LeadscrewStopState::SET) {
      return m_rightStopPosition;
    }
    return INT32_MAX;
  }
  return 0;
}

void Leadscrew::setCurrentPosition(int position) {
  m_currentPosition = position;
}


bool Leadscrew::sendPulse() {

#ifdef ELS_USE_RMT
  rmtWrite(rmtObj, rmt_data, sizeof(rmt_data));
  return true;
#else
  uint8_t pinState = m_io->readStepPin();

  // Keep the pulse pin high as long as we're not scheduled to send a pulse
  if (pinState == 1) {
    m_io->writeStepPin(0);

  } else {
    m_io->writeStepPin(1);
  }

  return pinState == 1;
#endif
}

/**
 * Due to the cumulative nature of the pulses when stopping, we can model the
 * stopping distance as a quadratic equation.
 * This function calculates the number of pulses required to stop the leadscrew
 * from a given pulse delay
 */
int Leadscrew::getStoppingDistanceInPulses() {
  float time = m_leadscrewSpeed / m_leadscrewAccel;
  return m_leadscrewSpeed * time / 2;
}

/**
 * This will be positive for decceleration, negative for accelleration.
 * 
 * This is not the absolute number of pulses, but the number of pulses gained/lost during the
 * accelleration/decelleration process. 
 */
int Leadscrew::getTargetSpeedDistanceInPulses() {
  float targetSpeed = m_spindle->getEstimatedVelocityInPPS() * m_ratio;
  float time = abs(m_leadscrewSpeed - targetSpeed) / m_leadscrewAccel;
  return ((m_leadscrewSpeed - targetSpeed) * time / 2);
}

void Leadscrew::update() {

  int64_t tm = micros();

  GlobalState* globalState = GlobalState::getInstance();
  GlobalMotionMode mode = globalState->getMotionMode();

  bool hitLeftEndstop = m_leftStopState == LeadscrewStopState::SET &&
    m_currentPosition <= m_leftStopPosition;

  bool hitRightEndstop = m_rightStopState == LeadscrewStopState::SET &&
    m_currentPosition >= m_rightStopPosition;

  if (mode == GlobalMotionMode::JOG_LEFT) {
    m_spindle->consumePosition(); // Consume the spindle position while we're jogging
    if (tm - this->jogMicros > ELS_JOG_PULSE_DELAY) {
      m_expectedPosition = m_currentPosition - 1500;
      this->jogMicros = tm;
    }
  } else if (mode == GlobalMotionMode::JOG_RIGHT) {
    m_spindle->consumePosition(); // Consume the spindle position while we're jogging
    if (tm - this->jogMicros > ELS_JOG_PULSE_DELAY) {
      m_expectedPosition = (m_currentPosition + 1500);
      this->jogMicros = tm;
    }
  } else {
    // Update expected position from any unconsumed spindle pulses
    m_expectedPosition = (m_expectedPosition + (float)(((float)m_spindle->consumePosition()) * m_ratio));
  }

  // How far are we from the expected position
  float positionError = getPositionError();
  if ((hitLeftEndstop || hitRightEndstop) && abs(positionError) > encoderPPR * m_ratio) {
    //DEBUG_F("Hit end stop %d %d %d %d %f %d %d %d\n", hitLeftEndstop, hitRightEndstop, positionError, encoderPPR, m_ratio, m_currentPosition, m_leftStopPosition, m_rightStopPosition);
    // if we've hit the endstop, keep the expected position within one spindle rotation of the endstop
    // we can assume that the current position will not actually move due to later logic

    // if the position error is bigger than one rev worht of movement, reset the expected so that we don't move

    m_expectedPosition = (m_currentPosition);
    if ((mode == GlobalMotionMode::JOG_LEFT && hitLeftEndstop)
      || (mode == GlobalMotionMode::JOG_RIGHT && hitRightEndstop)
      || (mode == GlobalMotionMode::MM_ENABLED && hitLeftEndstop)) {
      globalState->setMotionMode(GlobalMotionMode::MM_DISABLED);
      globalState->setThreadSyncState(GlobalThreadSyncState::UNSYNC);
    }
  }



  switch (globalState->getMotionMode()) {
  case GlobalMotionMode::MM_DISABLED:
    // consume position but don't move
    m_expectedPosition = (m_currentPosition);
    m_spindle->consumePosition();
    break;
  case GlobalMotionMode::JOG_LEFT:
  case GlobalMotionMode::JOG_RIGHT:
  case GlobalMotionMode::MM_ENABLED:
    LeadscrewDirection nextDirection = LeadscrewDirection::UNKNOWN;

    /**
     * Attempt to find the "next" direction to move in, if the current
     * direction is unknown i.e: at a standstill - we know we have to start
     * moving in that direction
     *
     * If the next direction is different from the current direction, we
     * should start decelerating to move in the intended direction
     */
    if (positionError > 1 && !hitRightEndstop) {
      nextDirection = LeadscrewDirection::RIGHT;
      if (m_currentDirection == LeadscrewDirection::LEFT && m_leadscrewSpeed == 0) {
        m_currentDirection = LeadscrewDirection::UNKNOWN;
      }
      if (m_currentDirection == LeadscrewDirection::UNKNOWN) {
        m_io->writeDirPin(ELS_DIR_RIGHT);
        m_currentDirection = LeadscrewDirection::RIGHT;
      }
    } else if (positionError < 1 && !hitLeftEndstop) {
      nextDirection = LeadscrewDirection::LEFT;
      if (m_currentDirection == LeadscrewDirection::RIGHT && m_leadscrewSpeed == 0) {
        m_currentDirection = LeadscrewDirection::UNKNOWN;
      }
      if (m_currentDirection == LeadscrewDirection::UNKNOWN) {
        m_io->writeDirPin(ELS_DIR_LEFT);
        m_currentDirection = LeadscrewDirection::LEFT;
      }
    } else {
      m_currentDirection = LeadscrewDirection::UNKNOWN;
    }


    /**
     * If we are not in sync with the thread, if not, figure out where we should restart based on
     * the difference in position between the sync point and the current position
     */
     // if(globalState->getThreadSyncState() == GlobalThreadSyncState::UNSYNC)DEBUG_F("Currently unsynced %s\n",
       //  m_syncPositionState == LeadscrewSpindleSyncPositionState::UNSET ? "UNSET" :  m_syncPositionState == LeadscrewSpindleSyncPositionState::LEFT ? "LEFT" : "RIGHT")
    if (m_syncPositionState != LeadscrewSpindleSyncPositionState::UNSET && globalState->getThreadSyncState() == GlobalThreadSyncState::UNSYNC) {
      int syncPosition = 0;
      switch (m_syncPositionState) {
      case LeadscrewSpindleSyncPositionState::LEFT:
        syncPosition = m_leftStopPosition;
        break;
      case LeadscrewSpindleSyncPositionState::RIGHT:
        syncPosition = m_rightStopPosition;
        break;
      case LeadscrewSpindleSyncPositionState::UNSET:  // Can we even hit this???
        // position does not matter
        syncPosition = m_currentPosition;
        break;
      }

      int currentpos = m_spindle->getCurrentPosition();

      if (globalState->getThreadSyncState() != GlobalThreadSyncState::SYNC) {
        int pulsesToTargetSpeed = getTargetSpeedDistanceInPulses();
        // So, I think this is, how far we need to move, converted to spindle pulses, plus the spindle sync pos, mod the spindle PPM, to get the next revolution. 
        int expectedSyncPosition = ((int)((m_currentPosition - syncPosition) / m_ratio) + m_spindleSyncPosition) % encoderPPR;


        if (currentpos == expectedSyncPosition) {
          m_expectedPosition = m_currentPosition; // Ensure these are aligned at the sync point. 
          globalState->setThreadSyncState(GlobalThreadSyncState::SYNC);
        }

      }
    }


    /**
     * determine if we should even be bothering to send a pulse
     * we know that we can short circuit this if:
     * - Our current direction is unknown
     * - The last pulse was sent recently i.e: less than the current pulse delay
     * - the sync position was previously set and we are currently not synced with the spindle
     */
    if (m_currentDirection == LeadscrewDirection::UNKNOWN
      || (tm - m_lastPulseTimestamp) < m_currentPulseDelay
      || (m_syncPositionState != LeadscrewSpindleSyncPositionState::UNSET && globalState->getThreadSyncState() == GlobalThreadSyncState::UNSYNC)) {
      break;
    }

    // attempt to keep in sync with the leadscrew
    // if sendPulse returns true, we've actually sent a pulse
    if (sendPulse()) {
      /**
       * If we've sent a pulse, we need to update the last pulse micros for velocity calculations
       */
      m_lastFullPulseDurationMicros = (uint32_t)(tm - m_lastPulseTimestamp);
      m_lastPulseTimestamp = tm;

      /**
       * If the pulse was sent, we need to update the accumulator to keep track of the position
       */
      m_currentPosition += static_cast<int>(m_currentDirection);
      /**
       * We need to determine if we need to start decelerating to stop at the designated position
       *
       * The conditions which we need to start decelerating are:
       * - The position error is less than the stopping distance
       * - The direction has changed
       * - We're going to hit an endstop set by the user
       *
       * Since we have no set "stop point" like with gcode or otherwise we need to constantly be updating
       * the stopping distance based on the current speed and acceleration and cant plan ahead much further than this
       */
      int pulsesToStop = getStoppingDistanceInPulses();
      int pulsesToTargetSpeed = getTargetSpeedDistanceInPulses();

      bool goingToHitLeftEndstop = m_leftStopState == LeadscrewStopState::SET &&
        m_currentPosition - pulsesToStop <= m_leftStopPosition;
      bool goingToHitRightEndstop = m_rightStopState == LeadscrewStopState::SET &&
        m_currentPosition + pulsesToStop >= m_rightStopPosition;

      // if this is true we should start decelerating to stop at the
      // correct position
      bool shouldStop = ((int)m_currentDirection * positionError) < pulsesToTargetSpeed ||
        nextDirection != m_currentDirection ||
        goingToHitLeftEndstop || goingToHitRightEndstop;


      if (shouldStop) {
        m_leadscrewSpeed -= m_leadscrewAccel * min(m_currentPulseDelay, initialPulseDelay) / US_PER_SECOND;
        m_leadscrewSpeed = max(m_leadscrewSpeed, (float)0); // don't let this go below zero 
        m_currentPulseDelay = m_leadscrewSpeed == 0 ? initialPulseDelay : US_PER_SECOND / m_leadscrewSpeed;
      } else {
        m_leadscrewSpeed += m_leadscrewAccel * min(m_currentPulseDelay, initialPulseDelay) / US_PER_SECOND;
        m_leadscrewSpeed = min(m_leadscrewSpeed, LEADSCREW_MAX_SPEED_PPS);
        m_currentPulseDelay = m_leadscrewSpeed == 0 ? initialPulseDelay : US_PER_SECOND / m_leadscrewSpeed;
      }

      /**
       * Ensure that the pulse delay is within the bounds
       * of the initial pulse delay (i.e the pulse delay when moving from zero) and 0
       */
      if (m_currentPulseDelay > initialPulseDelay) {
        m_currentPulseDelay = initialPulseDelay;
      }
      if (m_currentPulseDelay < 0) {
        m_currentPulseDelay = 0;
      }
    }

    break;
  }
}

int Leadscrew::getPositionError() {
  return m_expectedPosition - m_currentPosition;
}

LeadscrewDirection Leadscrew::getCurrentDirection() {
  return m_currentDirection;
}

float Leadscrew::getEstimatedVelocityInMillimetersPerSecond() {
  return (getEstimatedVelocityInPulsesPerSecond() * leadscrewPitch) /
    motorPulsePerRevolution;
}
