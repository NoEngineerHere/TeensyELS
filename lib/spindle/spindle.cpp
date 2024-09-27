#include "spindle.h"

#include <config.h>
#include <els_elapsedMillis.h>
#include <math.h>

#ifndef ELS_SPINDLE_DRIVEN
Spindle::Spindle(int pinA, int pinB) : m_encoder(pinA, pinB) {
#else
Spindle::Spindle() {
#endif

  m_unconsumedPosition = 0;
  m_lastPulseMicros = 0;
  m_lastFullPulseDurationMicros = 0;
  m_currentPosition = 0;
}

void Spindle::update() {
  // read the encoder and update the current position
  // todo: we should keep the absolute position of the spindle, cbf right now
  int position = m_encoder.read();
  incrementCurrentPosition(position);
  m_encoder.write(0);
}

void Spindle::setCurrentPosition(int position) {
  // update the unconsumed position by finding the delta between the old and new
  // positions
  int positionDelta = position - m_currentPosition;
  m_unconsumedPosition += positionDelta;

  m_currentPosition = position % ELS_SPINDLE_ENCODER_PPR;
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