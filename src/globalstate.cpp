#include "globalstate.h"

#include <Wire.h>

GlobalState *GlobalState::m_instance = nullptr;
GlobalState *GlobalState::getInstance() {
  if (m_instance == nullptr) {
    m_instance = new GlobalState();
  }
  return m_instance;
}

void GlobalState::printState() {
  Serial.print("Drive Mode: ");
  switch (m_motionMode) {
    case DISABLED:
      Serial.println("DISABLED");
      break;
    case ENABLED:
      Serial.println("ENABLED");
      break;
    case JOG:
      Serial.println("JOG");
      break;
  }
  Serial.print("Feed Mode: ");
  switch (m_feedMode) {
    case FEED:
      Serial.println("FEED");
      break;
    case THREAD:
      Serial.println("THREAD");
      break;
  }
  Serial.print("Unit Mode: ");
  switch (m_unitMode) {
    case METRIC:
      Serial.println("METRIC");
      break;
    case IMPERIAL:
      Serial.println("IMPERIAL");
      break;
  }
  Serial.print("Thread Sync State: ");
  switch (m_threadSyncState) {
    case SYNC:
      Serial.println("SYNC");
      break;
    case UNSYNC:
      Serial.println("UNSYNC");
      break;
  }
  Serial.print("Leadscrew position: ");
  Serial.println(m_leadscrew.getCurrentPosition());
  Serial.print("Leadscrew expected position: ");
  Serial.println(m_leadscrew.getExpectedPosition());
  Serial.print("Leadscrew ratio: ");
  Serial.println(m_leadscrew.getRatio());
  Serial.print("Spindle position: ");
  Serial.println(m_spindle.getCurrentPosition());
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

  // update leadscrew ratio
  m_leadscrew.setRatio(getCurrentFeedPitch());
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

void GlobalState::incrementSpindlePosition(int amount) {
  m_spindle.incrementCurrentPosition(amount);
}

void GlobalState::resetSpindlePosition() { m_spindle.resetCurrentPosition(); }

int GlobalState::getLeadscrewPositionError() {
  return m_leadscrew.getExpectedPosition() - m_leadscrew.getCurrentPosition();
}

void GlobalState::incrementLeadscrewPosition(int amount) {
  m_leadscrew.incrementCurrentPosition(amount);
}

void GlobalState::resetLeadscrewPosition() {
  m_leadscrew.resetCurrentPosition();
}

float GlobalState::getLeadscrewStepAccumulator() {
  return (ELS_LEADSCREW_STEPS_PER_MM * m_leadscrew.getRatio()) /
         ELS_LEADSCREW_STEPPER_PPR;
}

void GlobalState::setMotionMode(GlobalMotionMode mode) { m_motionMode = mode; }

GlobalMotionMode GlobalState::getMotionMode() { return m_motionMode; }

void GlobalState::setUnitMode(GlobalUnitMode mode) { m_unitMode = mode; }

GlobalUnitMode GlobalState::getUnitMode() { return m_unitMode; }

void GlobalState::setThreadSyncState(GlobalThreadSyncState state) {
  m_threadSyncState = state;
}

GlobalThreadSyncState GlobalState::getThreadSyncState() {
  return m_threadSyncState;
}

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