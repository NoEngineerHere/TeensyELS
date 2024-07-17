#include "spindle.h"

int Spindle::getCurrentPosition() { return m_currentPosition; }

void Spindle::resetCurrentPosition() { m_currentPosition = 0; }

void Spindle::setCurrentPosition(int position) { m_currentPosition = position; }
void Spindle::incrementCurrentPosition(int amount) {
  m_currentPosition += amount;
}
