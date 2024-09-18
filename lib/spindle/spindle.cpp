#include "spindle.h"

#include <config.h>
#include <els_elapsedMillis.h>
#include <math.h>

void Spindle::setCurrentPosition(int position) {
  int newPosition = position % ELS_SPINDLE_ENCODER_PPR;
  m_unconsumedPosition = newPosition - m_currentPosition;
  m_currentPosition = newPosition;
}

void Spindle::incrementCurrentPosition(int amount) {
  setCurrentPosition(getCurrentPosition() + amount);
  if (amount != 0) {
    m_lastFullPulseDurationMicros = m_lastPulseMicros / abs(amount);
    m_lastPulseMicros = 0;
  }
}

float Spindle::getEstimatedVelocityInRPM() {
  return getEstimatedVelocityInPulsesPerSecond() / ELS_SPINDLE_ENCODER_PPR;
}

int Spindle::consumePosition() {
  int position = m_unconsumedPosition;
  m_unconsumedPosition = 0;
  return position;
}