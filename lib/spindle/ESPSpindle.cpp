#ifdef ESP32
#include "Spindle.h"

#include <config.h>
#include <math.h>

#ifndef ELS_SPINDLE_DRIVEN
Spindle::Spindle(int pinA, int pinB) : m_encoder() {
#else
Spindle::Spindle() {
#endif
  ESP32Encoder::useInternalWeakPullResistors = puType::none;
  m_encoder.attachFullQuad(pinA, pinB);
  gpio_pullup_en((gpio_num_t)pinA);
  gpio_pullup_en((gpio_num_t)pinB);

  m_unconsumedPosition = 0;
  m_lastPulseTimestamp = micros();
  m_lastFullPulseDurationMicros = 0;
  m_currentPosition = 0;
}

void Spindle::update() {
  // read the encoder and update the current position
  // todo: we should keep the absolute position of the spindle, cbf right now
  int64_t position = m_encoder.clearCount();
  incrementCurrentPosition(position);
}

void Spindle::setCurrentPosition(int position) {
  // update the unconsumed position by finding the delta between the old and new
  // positions
  int positionDelta = position - m_currentPosition;
  m_unconsumedPosition += positionDelta;

  m_currentPosition = (position + ELS_SPINDLE_ENCODER_PPR) % ELS_SPINDLE_ENCODER_PPR;
}

void Spindle::incrementCurrentPosition(int amount) {
  int64_t t = micros();
  int pos = getCurrentPosition() + amount;
  setCurrentPosition(pos);
  int newpos = getCurrentPosition();
  if (pos != newpos) // spindle pos has wrapped
  {
    m_lastRevPosition -= pos - newpos;
  }
  if (amount != 0) {
    m_lastPulseTimestamp = t;
    if (abs(newpos - m_lastRevPosition) > ELS_SPEED_COUNTS) {
      // Update stats for last full revolution. 
      m_lastRevSize = newpos - m_lastRevPosition;
      m_lastRevPosition = newpos;
      m_lastRevMicros = t - m_lastRevTimestamp;
      m_lastRevTimestamp = t;
    }
  }
}

float Spindle::getEstimatedVelocityInPPS() {
  if (m_lastRevMicros == 0)return 0;
  if (micros() - m_lastRevTimestamp > 1000000)return 0;
  return abs((m_lastRevSize * US_PER_SECOND) / (m_lastRevMicros));
}


float Spindle::getEstimatedVelocityInRPM() {
  if (m_lastRevMicros == 0)return 0;
  if (micros() - m_lastRevTimestamp > 1000000)return 0;
  return abs((m_lastRevSize * 60000000) / (m_lastRevMicros * ELS_SPINDLE_ENCODER_PPR));
}

int Spindle::consumePosition() {
  int position = m_unconsumedPosition;
  m_unconsumedPosition = 0;
  return position;
}
#endif