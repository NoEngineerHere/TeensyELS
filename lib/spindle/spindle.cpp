#include "spindle.h"

#include <config.h>
#include <els_elapsedMillis.h>
#include <math.h>

void Spindle::incrementCurrentPosition(int amount) {
  setCurrentPosition(getCurrentPosition() + amount);
  if (amount != 0) {
    m_lastFullPulseDurationMicros = m_lastPulseMicros / abs(amount);
    m_lastPulseMicros = 0;
  }
  // limit the position to one rotation if the limits are set
  if (m_leftLimitState == SpindleLimitState::SET &&
      getCurrentPosition() < m_leftLimitPosition - ELS_SPINDLE_ENCODER_PPR) {
    setCurrentPosition(m_leftLimitPosition);
  }
  if (m_rightLimitState == SpindleLimitState::SET &&
      getCurrentPosition() > m_rightLimitPosition + ELS_SPINDLE_ENCODER_PPR) {
    setCurrentPosition(m_rightLimitPosition);
  }
}

float Spindle::getEstimatedVelocityInRPM() {
  return getEstimatedVelocityInPulsesPerSecond() / ELS_SPINDLE_ENCODER_PPR;
}

void Spindle::setPositionLimit(SpindleLimitOption option, int position) {
  switch (option) {
    case LEFT:
      m_leftLimitPosition = position;
      m_leftLimitState = SpindleLimitState::SET;
      break;
    case RIGHT:
      m_rightLimitPosition = position;
      m_rightLimitState = SpindleLimitState::SET;
      break;
  }
}

void Spindle::unsetPositionLimit(SpindleLimitOption option) {
  switch (option) {
    case LEFT:
      m_leftLimitState = SpindleLimitState::UNSET;
      m_leftLimitPosition = INT32_MIN;
      break;
    case RIGHT:
      m_rightLimitState = SpindleLimitState::UNSET;
      m_rightLimitPosition = INT32_MAX;
      break;
  }
}
