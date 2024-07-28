#include "spindle.h"

#include <elapsedMillis.h>

#include "config.h"

int Spindle::getCurrentPosition() { return m_currentPosition; }

void Spindle::resetCurrentPosition() { m_currentPosition = 0; }

void Spindle::setCurrentPosition(int position) { m_currentPosition = position; }
void Spindle::incrementCurrentPosition(int amount) {
  m_currentPosition += amount;
  m_lastPulseMicros = micros();
}

float Spindle::getEstimatedVelocityInPulsesPerSecond() {
  return (float)(1000000 / m_lastPulseMicros);
}

float Spindle::getEstimatedVelocityInRPM() {
  return getEstimatedVelocityInPulsesPerSecond() / ELS_SPINDLE_ENCODER_PPR;
}
