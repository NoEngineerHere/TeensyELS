#include "globalstate.h"

GlobalState *GlobalState::m_instance = nullptr;
GlobalState *GlobalState::getInstance() {
  if (m_instance == nullptr) {
    m_instance = new GlobalState();
  }
  return m_instance;
}

void GlobalState::setMode(GlobalMajorMode mode) { m_mode = mode; }

GlobalMajorMode GlobalState::getMode() { return m_mode; }

void GlobalState::setMotionMode(GlobalMotionMode mode) { m_minorMode = mode; }

GlobalMotionMode GlobalState::getMotionMode() { return m_minorMode; }

void GlobalState::setStopPosition(int stop, int position) {
  if (stop == 1) {
    m_stop1Position = position;
  } else if (stop == 2) {
    m_stop2Position = position;
  }
}

int GlobalState::getStopPosition(int stop) {
  if (stop == 1) {
    return m_stop1Position;
  } else if (stop == 2) {
    return m_stop2Position;
  }
}