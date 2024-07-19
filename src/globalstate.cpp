#include "globalstate.h"

GlobalState *GlobalState::m_instance = nullptr;
GlobalState *GlobalState::getInstance() {
  if (m_instance == nullptr) {
    m_instance = new GlobalState();
  }
  return m_instance;
}

void GlobalState::setFeedMode(GlobalFeedMode mode) {
  m_feedMode = mode;

  // when switching feed modes ensure that the default for the next mode is
  // selected via setFeedSelect - depends on the fallback in the function
  setFeedSelect(-1);
}

GlobalFeedMode GlobalState::getFeedMode() { return m_feedMode; }

int GlobalState::getFeedSelect() { return m_feedSelect; }
int GlobalState::getCurrentFeedSelectArraySize() {
  // this just ensures that the feedSelect doesn't go out of bounds for the
  // current arry
  if (m_unitMode == METRIC) {
    if (m_feedMode == THREAD) {
      return ARRAY_SIZE(threadPitchMetric);
    } else {
      return ARRAY_SIZE(feedPitchMetric);
    }
  } else {
    if (m_feedMode == THREAD) {
      return ARRAY_SIZE(threadPitchImperial);
    } else {
      return ARRAY_SIZE(feedPitchImperial);
    }
  }

  // invalid - should never get here!
  return -1;
}

void GlobalState::setFeedSelect(int select) {
  if (select >= 0 && select < getCurrentFeedSelectArraySize()) {
    m_feedSelect = select;
  } else {
    // if we're out of bounds, just set the default
    if (m_feedMode == THREAD) {
      if (m_unitMode == METRIC) {
        m_feedSelect = DEFAULT_METRIC_THREAD_PITCH_IDX;
      } else {
        m_feedSelect = DEFAULT_IMPERIAL_THREAD_PITCH_IDX;
      }
    } else {
      if (m_unitMode == METRIC) {
        m_feedSelect = DEFAULT_METRIC_FEED_PITCH_IDX;
      } else {
        m_feedSelect = DEFAULT_IMPERIAL_FEED_PITCH_IDX;
      }
    }
  }
}

float GlobalState::getCurrentFeedPitch() {
  if (m_unitMode == METRIC) {
    if (m_feedMode == THREAD) {
      return threadPitchMetric[m_feedSelect];
    } else {
      return feedPitchMetric[m_feedSelect];
    }
  }

  // special cases for imperial
  if (m_feedMode == THREAD) {
    // threads are defined in TPI, not pitch
    return (1.0 / threadPitchImperial[m_feedSelect]) * 25.4;
  }
  // feeds are defined in thou/rev, not mm/rev
  return feedPitchImperial[m_feedSelect] * 25.4 / 1000;
}

int GlobalState::nextFeedPitch() {
  if (m_feedSelect != getCurrentFeedSelectArraySize() - 1) {
    setFeedSelect(m_feedSelect + 1);
  }

  return m_feedSelect;
}

int GlobalState::prevFeedPitch() {
  if (m_feedSelect != 0) {
    setFeedSelect(m_feedSelect - 1);
  }

  return m_feedSelect;
}

void GlobalState::setMotionMode(GlobalMotionMode mode) { m_minorMode = mode; }

GlobalMotionMode GlobalState::getMotionMode() { return m_minorMode; }

void GlobalState::setUnitMode(GlobalUnitMode mode) { m_unitMode = mode; }

GlobalUnitMode GlobalState::getUnitMode() { return m_unitMode; }

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