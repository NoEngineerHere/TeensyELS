#include <globalstate.h>
#include <critical_section.h>

void GlobalState::setFeedMode(GlobalFeedMode mode) {
  ElsCriticalSection guard;
  m_feedMode = mode;

  // when switching feed modes ensure that the default for the next mode is
  // selected via setFeedSelect - depends on the fallback in the function
  setFeedSelectUnlocked(-1);
}

GlobalState::FeedSelection GlobalState::feedSelection() const {
  ElsCriticalSection guard;
  return {m_feedMode, m_unitMode, m_feedSelect};
}
GlobalFeedMode GlobalState::getFeedMode() { return feedSelection().mode; }
int GlobalState::getFeedSelect() { return feedSelection().index; }
int GlobalState::getCurrentFeedSelectArraySize() {
  // this just ensures that the feedSelect doesn't go out of bounds for the
  // current arry
  if (m_unitMode == METRIC) {
    if (m_feedMode == FM_THREAD) {
      return ARRAY_SIZE(threadPitchMetric);
    } else {
      return ARRAY_SIZE(feedPitchMetric);
    }
  } else {
    if (m_feedMode == FM_THREAD) {
      return ARRAY_SIZE(threadPitchImperial);
    } else {
      return ARRAY_SIZE(feedPitchImperial);
    }
  }

  // invalid - should never get here!
  return -1;
}

void GlobalState::setButtonLock(GlobalButtonLock lock) {
  ElsCriticalSection guard;
  m_buttonLock = lock;
}
GlobalButtonLock GlobalState::getButtonLock() {
  ElsCriticalSection guard;
  return m_buttonLock;
}

void GlobalState::setFeedSelect(int select) {
  ElsCriticalSection guard;
  setFeedSelectUnlocked(select);
}
void GlobalState::setFeedSelectUnlocked(int select) {
  if (select >= 0 && select < getCurrentFeedSelectArraySize()) {
    m_feedSelect = select;
  } else {
    // if we're out of bounds, just set the default
    if (m_feedMode == FM_THREAD) {
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
  const auto selection = feedSelection();
  const auto index = selection.index;
  if (selection.unit == METRIC)
    return selection.mode == FM_THREAD ? threadPitchMetric[index] : feedPitchMetric[index];
  if (selection.mode == FM_THREAD) return (1.0 / threadPitchImperial[index]) * 25.4;
  return feedPitchImperial[index] * 25.4 / 1000;
}

int GlobalState::nextFeedPitch() {
  ElsCriticalSection guard;
  if (m_feedSelect != getCurrentFeedSelectArraySize() - 1) ++m_feedSelect;
  return m_feedSelect;
}
int GlobalState::prevFeedPitch() {
  ElsCriticalSection guard;
  if (m_feedSelect != 0) --m_feedSelect;
  return m_feedSelect;
}

void GlobalState::requestMotionUnlocked(GlobalMotionMode mode) {
  if (m_updateArmed.load(std::memory_order_relaxed) && mode != MM_DISABLED) return;
  if (mode != m_motionMode) m_motionResetRequested = true;
  m_motionMode = mode;
}
void GlobalState::setMotionMode(GlobalMotionMode mode) {
  ElsCriticalSection guard;
  requestMotionUnlocked(mode);
}
GlobalMotionMode GlobalState::getMotionMode() {
  ElsCriticalSection guard;
  return m_motionMode;
}
GlobalMotionMode GlobalState::takeMotionRequest(bool& reset) {
  ElsCriticalSection guard;
  reset = m_motionResetRequested;
  m_motionResetRequested = false;
  return m_motionMode;
}

void GlobalState::setUnitMode(GlobalUnitMode mode) {
  ElsCriticalSection guard;
  m_unitMode = mode;
}
GlobalUnitMode GlobalState::getUnitMode() { return feedSelection().unit; }

void GlobalState::setThreadSyncState(GlobalThreadSyncState state) {
  m_threadSyncState = state;
}

GlobalThreadSyncState GlobalState::getThreadSyncState() {
  return m_threadSyncState;
}

UpdateStatus GlobalState::updateStatus() const {
  ElsCriticalSection guard;
  return m_updateStatus;
}
void GlobalState::publishUpdate(const UpdateStatus& status) {
  ElsCriticalSection guard;
  m_updateStatus = status;
}

void  GlobalState::setDisplayReset() { m_displayReset = true; }
bool GlobalState::getDisplayReset() {
  return m_displayReset.exchange(false);
}
