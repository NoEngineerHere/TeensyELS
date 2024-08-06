#include "spindle.h"

#include <config.h>
#include <els_elapsedMillis.h>
#include <math.h>

Spindle::Spindle() {
  m_lastPulseMicros = 0;
  m_lastFullPulseDurationMicros = 0;
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
