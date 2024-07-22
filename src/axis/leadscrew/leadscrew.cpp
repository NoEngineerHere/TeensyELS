#include "leadscrew.h"

#include <Wire.h>

#include "../../globalstate.h"

Leadscrew::Leadscrew(Axis* leadAxis) : DerivedAxis() {
  m_leadAxis = leadAxis;
  m_ratio = 1.0;
  m_currentPosition = 0;
  m_lastPulseMicros = 0;
  m_currentPulseDelay = LEADSCREW_INITIAL_PULSE_DELAY_US;
}

void Leadscrew::setRatio(float ratio) {
  m_ratio = ratio;
  // extrapolate the current position based on the new ratio
  m_currentPosition = m_leadAxis->getCurrentPosition() * m_ratio;
}

float Leadscrew::getRatio() { return m_ratio; }

int Leadscrew::getExpectedPosition() {
  return m_leadAxis->getCurrentPosition() * m_ratio;
}

int Leadscrew::getCurrentPosition() { return m_currentPosition; }

void Leadscrew::resetCurrentPosition() {
  m_currentPosition = m_leadAxis->getCurrentPosition() * m_ratio;
}

void Leadscrew::setCurrentPosition(int position) {
  m_currentPosition = position;
}

void Leadscrew::incrementCurrentPosition(int amount) {
  m_currentPosition += amount;
}

float Leadscrew::getAccumulatorUnit() {
  return (ELS_LEADSCREW_STEPS_PER_MM * getRatio()) / ELS_LEADSCREW_STEPPER_PPR;
}

void Leadscrew::sendPulse() {
  if (digitalReadFast(2) == HIGH) {
    digitalWriteFast(2, LOW);
    m_lastPulseMicros = micros();

    if (m_accumulator > 1) {
      m_accumulator--;
    }
    if (m_accumulator < -1) {
      m_accumulator++;
    }
  } else {
    digitalWriteFast(2, HIGH);
  }
}

void Leadscrew::update() {
  GlobalState* globalState = GlobalState::getInstance();

  int currentMicros = micros();

  int positionError = getExpectedPosition() - getCurrentPosition();

  int directionIncrement = 0;

  if (positionError > 0) {
    digitalWriteFast(3, HIGH);
    directionIncrement = 1;
  } else if (positionError < 0) {
    digitalWriteFast(3, LOW);
    directionIncrement = -1;
  }

  switch (globalState->getMotionMode()) {
    case GlobalMotionMode::DISABLED:
      // ignore the spindle, pretend we're in sync all the time
      resetCurrentPosition();
      break;
    case GlobalMotionMode::JOG:
      // only send a pulse if we haven't sent one recently
      if (currentMicros - m_lastPulseMicros < JOG_PULSE_DELAY_US) {
        break;
      }
      // if jog is complete go back to disabled motion mode
      if (positionError == 0) {
        globalState->setMotionMode(GlobalMotionMode::DISABLED);
      }
      // jogging is a fixed rate based on JOG_PULSE_DELAY_US
      sendPulse();

      break;
    case GlobalMotionMode::ENABLED:

      if (positionError == 0) {
        // todo check if we have sync'd before
        globalState->setThreadSyncState(GlobalThreadSyncState::SYNC);
        break;
      }

      if (currentMicros - m_lastPulseMicros < 100) {
        break;
      }

      // attempt to keep in sync with the leadscrew

      sendPulse();

      if ((int)(m_accumulator) == 0) {
        incrementCurrentPosition(directionIncrement);
      } else {
        m_accumulator += getAccumulatorUnit();
        Serial.print("Accumulator: ");
        Serial.println(m_accumulator);
      }

      m_lastPulseMicros = currentMicros;

      break;
  }
}
