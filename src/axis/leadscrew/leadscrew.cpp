#include "leadscrew.h"

Leadscrew::Leadscrew(Axis* leadAxis) : DerivedAxis() {
  m_leadAxis = leadAxis;
  m_ratio = 1.0;
  m_currentPosition = 0;
}

void Leadscrew::setRatio(float ratio) { m_ratio = ratio; }

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
