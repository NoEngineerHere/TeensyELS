#include "leadscrew.h"

#include <globalstate.h>

#include <cmath>
#include <cstdint>
#include <cstdio>

#include "leadscrew_io.h"
using namespace std;

Leadscrew::Leadscrew(Axis* leadAxis, LeadscrewIO* io, float initialPulseDelay,
                     float pulseDelayIncrement, int motorPulsePerRevolution,
                     float leadscrewPitch)
    : motorPulsePerRevolution(motorPulsePerRevolution),
      leadscrewPitch(leadscrewPitch),
      initialPulseDelay(initialPulseDelay),
      pulseDelayIncrement(pulseDelayIncrement),
      m_io(io),
      m_leadAxis(leadAxis),
      m_accumulator(0),
      m_currentDirection(LeadscrewDirection::UNKNOWN),
      m_leftStopState(LeadscrewStopState::UNSET),
      m_rightStopState(LeadscrewStopState::UNSET),
      m_currentPulseDelay(initialPulseDelay) {
  setRatio(GlobalState::getInstance()->getCurrentFeedPitch());
  m_lastPulseMicros = 0;
  m_lastFullPulseDurationMicros = 0;
  m_currentPosition = 0;
}

void Leadscrew::setRatio(float ratio) {
  m_ratio = ratio;
  // extrapolate the current position based on the new ratio
  m_currentPosition = m_leadAxis->getCurrentPosition() * m_ratio;
  if (m_leftStopState == LeadscrewStopState::SET) {
    m_leftStopPosition = m_leftStopPosition * m_ratio;
  }
  if (m_rightStopState == LeadscrewStopState::SET) {
    m_rightStopPosition = m_rightStopPosition * m_ratio;
  }
  m_cycleModulo = motorPulsePerRevolution * m_ratio;
}

float Leadscrew::getRatio() { return m_ratio; }

int Leadscrew::getExpectedPosition() {
  return m_leadAxis->getCurrentPosition() * m_ratio;
}

int Leadscrew::getCurrentPosition() { return m_currentPosition; }

void Leadscrew::resetCurrentPosition() {
  m_currentPosition = m_leadAxis->getCurrentPosition() * m_ratio;
}

void Leadscrew::unsetStopPosition(StopPosition position) {
  switch (position) {
    case LEFT:
      m_leftStopState = LeadscrewStopState::UNSET;
      m_leftStopPosition = INT32_MIN;
      break;
    case RIGHT:
      m_rightStopState = LeadscrewStopState::UNSET;
      m_rightStopPosition = INT32_MAX;
      break;
  }
}

void Leadscrew::setStopPosition(StopPosition position, int stopPosition) {
  switch (position) {
    case LEFT:
      m_leftStopPosition = stopPosition;
      m_leftStopState = LeadscrewStopState::SET;
      break;
    case RIGHT:
      m_rightStopPosition = stopPosition;
      m_rightStopState = LeadscrewStopState::SET;
      break;
  }
}

LeadscrewStopState Leadscrew::getStopPositionState(StopPosition position) {
  switch (position) {
    case LEFT:
      return m_leftStopState;
    case RIGHT:
      return m_rightStopState;
  }
}

int Leadscrew::getStopPosition(StopPosition position) {
  // todo better default values when unset
  switch (position) {
    case LEFT:
      if (m_leftStopState == LeadscrewStopState::SET) {
        return m_leftStopPosition;
      }
      return INT32_MIN;
    case RIGHT:
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

void Leadscrew::incrementCurrentPosition(int amount) {
  m_currentPosition += amount;
}

float Leadscrew::getAccumulatorUnit() { return getRatio() / leadscrewPitch; }

bool Leadscrew::sendPulse() {
  uint8_t pinState = m_io->readStepPin();

  // Keep the pulse pin high as long as we're not scheduled to send a pulse
  if (pinState == 1) {
    m_io->writeStepPin(0);

  } else {
    m_io->writeStepPin(1);
  }

  return pinState == 1;
}

/**
 * Due to the cumulative nature of the pulses when stopping, we can model the
 * stopping distance as a quadratic equation.
 * This function calculates the number of pulses required to stop the leadscrew
 * from a given pulse delay
 */
int calculate_pulses_to_stop(float currentPulseDelay, float initialPulseDelay,
                             float pulseDelayIncrement) {
  // Calculate the discriminant
  float discriminant = currentPulseDelay * currentPulseDelay -
                       4 * (pulseDelayIncrement / 2.0) * (-initialPulseDelay);

  // Ensure the discriminant is non-negative for real roots
  if (discriminant >= 0) {
    // Calculate the square root of the discriminant
    float sqrtDiscriminant = sqrt(discriminant);

    // Calculate the positive root using the quadratic formula
    float n =
        (-currentPulseDelay + sqrtDiscriminant) / (2 * pulseDelayIncrement);

    // Round up to the nearest integer because pulses must be whole numbers
    return abs((int)ceil(n));
  } else {
    // If the discriminant is negative, return 0 as a fallback (no real
    // solution)
    return 0;
  }
}

void Leadscrew::update() {
  GlobalState* globalState = GlobalState::getInstance();

  int positionError = getPositionError();

  switch (globalState->getMotionMode()) {
    case GlobalMotionMode::DISABLED:
      // ignore the spindle, pretend we're in sync all the time
      resetCurrentPosition();
      break;
    case GlobalMotionMode::JOG:
    case GlobalMotionMode::ENABLED:
      LeadscrewDirection nextDirection = LeadscrewDirection::UNKNOWN;

      /**
       * Attempt to find the "next" direction to move in, if the current
       * direction is unknown i.e: at a standstill - we know we have to start
       * moving in that direction
       *
       * If the next direction is different from the current direction, we
       * should start decelerating to move in the intended direction
       */
      if (positionError > 0) {
        nextDirection = LeadscrewDirection::RIGHT;
        if (m_currentDirection == LeadscrewDirection::LEFT &&
            m_currentPulseDelay == initialPulseDelay) {
          m_currentDirection = LeadscrewDirection::UNKNOWN;
        }
        if (m_currentDirection == LeadscrewDirection::UNKNOWN) {
          m_io->writeDirPin(1);
          m_currentDirection = LeadscrewDirection::RIGHT;
          m_accumulator = m_currentDirection * getAccumulatorUnit();
        }

      } else if (positionError < 0) {
        nextDirection = LeadscrewDirection::LEFT;
        if (m_currentDirection == LeadscrewDirection::RIGHT &&
            m_currentPulseDelay == initialPulseDelay) {
          m_currentDirection = LeadscrewDirection::UNKNOWN;
        }
        if (m_currentDirection == LeadscrewDirection::UNKNOWN) {
          m_io->writeDirPin(0);
          m_currentDirection = LeadscrewDirection::LEFT;
          m_accumulator = m_currentDirection * getAccumulatorUnit();
        }
      } else {
        m_currentDirection = LeadscrewDirection::UNKNOWN;
        break;
      }

      bool hitEndstop = (m_rightStopState == LeadscrewStopState::SET &&
                         m_currentPosition >= m_rightStopPosition &&
                         m_currentDirection == LeadscrewDirection::RIGHT) ||
                        (m_leftStopState == LeadscrewStopState::SET &&
                         m_currentPosition <= m_leftStopPosition &&
                         m_currentDirection == LeadscrewDirection::LEFT);

      // check if we're scheduled for a pulse
      if (m_lastPulseMicros < m_currentPulseDelay || hitEndstop) {
        break;
      }

      // attempt to keep in sync with the leadscrew
      // if sendPulse returns true, we've actually sent a pulse
      if (sendPulse()) {
        m_lastFullPulseDurationMicros =
            min((uint32_t)m_lastPulseMicros, (uint32_t)initialPulseDelay);
        m_lastPulseMicros = 0;

        // handle position update
        if (m_accumulator > 1 || m_accumulator < -1) {
          m_accumulator -= m_currentDirection;
        } else {
          m_currentPosition += m_currentDirection;
          m_accumulator += m_currentDirection * getAccumulatorUnit();
        }

        // calculate the stopping time
        int pulsesToStop = calculate_pulses_to_stop(
            m_currentPulseDelay, initialPulseDelay, pulseDelayIncrement);

        // if this is true we should start decelerating to stop at the
        // correct position
        bool shouldStop = abs(positionError) <= pulsesToStop ||
                          nextDirection != m_currentDirection || hitEndstop;

        float accelChange = pulseDelayIncrement * m_lastFullPulseDurationMicros;

        if (shouldStop) {
          m_currentPulseDelay += accelChange;
        } else {
          m_currentPulseDelay -= accelChange;
        }

        // if pulse is sent we want to calculate how much to change the timing
        // for the next pulse
        // depending on accel and current speed etc
        // inital pulse delay is upper timing limit
        //
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
  return getExpectedPosition() - getCurrentPosition();
}

LeadscrewDirection Leadscrew::getCurrentDirection() {
  return m_currentDirection;
}

float Leadscrew::getEstimatedVelocityInMillimetersPerSecond() {
  return (getEstimatedVelocityInPulsesPerSecond() * leadscrewPitch) /
         motorPulsePerRevolution;
}

void Leadscrew::printState() {
  Serial.print("Leadscrew position: ");
  Serial.println(getCurrentPosition());
  Serial.print("Leadscrew expected position: ");
  Serial.println(getExpectedPosition());
  Serial.print("Leadscrew left stop position: ");
  Serial.println(getStopPosition(Leadscrew::StopPosition::LEFT));
  Serial.print("Leadscrew right stop position: ");
  Serial.println(getStopPosition(Leadscrew::StopPosition::RIGHT));
  Serial.print("Leadscrew ratio: ");
  Serial.println(getRatio());
  Serial.print("Leadscrew accumulator unit:");
  Serial.println(getAccumulatorUnit());
  Serial.print("Current leadscrew accumulator: ");
  Serial.println(m_accumulator);
  Serial.print("Leadscrew direction: ");
  switch (getCurrentDirection()) {
    case LeadscrewDirection::LEFT:
      Serial.println("LEFT");
      break;
    case LeadscrewDirection::RIGHT:
      Serial.println("RIGHT");
      break;
    case LeadscrewDirection::UNKNOWN:
      Serial.println("UNKNOWN");
      break;
  }
  Serial.print("Leadscrew current pulse delay: ");
  Serial.println(m_currentPulseDelay);
  Serial.print("Leadscrew position error: ");
  Serial.println(getPositionError());
  Serial.print("Leadscrew estimated velocity: ");
  Serial.println(getEstimatedVelocityInMillimetersPerSecond());
  Serial.print("Leadscrew pulses to stop: ");
  Serial.println(calculate_pulses_to_stop(
      m_currentPulseDelay, initialPulseDelay, pulseDelayIncrement));
}
