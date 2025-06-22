#include <config.h>

#if ELS_BOARD == ELS_BOARD_UNSET
#include "spindle.h"

#include <config.h>
#include <math.h>

Spindle::Spindle() {

  m_unconsumedPosition = 0;
  m_lastFullPulseDurationMicros = 0;
  m_currentPosition = 0;


}

void Spindle::update() {
  // read the encoder and update the current position
  // todo: we should keep the absolute position of the spindle, cbf right now
  //incrementCurrentPosition(position);
}

void Spindle::setCurrentPosition(int position) {
  // update the unconsumed position by finding the delta between the old and new
  // positions
  int positionDelta = position - m_currentPosition;
  m_unconsumedPosition += positionDelta;

  m_currentPosition = position % ELS_SPINDLE_ENCODER_PPR;
}

void Spindle::incrementCurrentPosition(int amount) {
  int pos = getCurrentPosition() + amount;
  setCurrentPosition(pos);
  int newpos = getCurrentPosition();

}

float Spindle::getEstimatedVelocityInRPM() {
  return 0.0;
}

int Spindle::consumePosition() {
  return -1;
}
#endif