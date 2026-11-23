#include "controls.h"

void Controls::changePitch(int64_t increments) {
  if (locked()) return;
  while (increments > 0) {
    state.nextFeedPitch();
    screw.setTargetPitchMM(state.getCurrentFeedPitch());
    --increments;
  }
  while (increments < 0) {
    state.prevFeedPitch();
    screw.setTargetPitchMM(state.getCurrentFeedPitch());
    ++increments;
  }
}

void Controls::toggleFeedMode() {
  if (locked()) return;
  switch (state.getFeedMode()) {
    case FM_FEED: state.setFeedMode(FM_THREAD); break;
    case FM_THREAD: state.setFeedMode(FM_FEED); break;
    case FM_UNSET: break;
  }
  screw.setTargetPitchMM(state.getCurrentFeedPitch());
}

void Controls::toggleUnits() {
  if (locked()) return;
  state.setUnitMode(state.getUnitMode() == METRIC ? IMPERIAL : METRIC);
  screw.setTargetPitchMM(state.getCurrentFeedPitch());
}

void Controls::toggleLock() {
  state.setButtonLock(locked() ? LK_UNLOCKED : LK_LOCKED);
}

void Controls::toggleMotion() {
  if (locked()) return;
  const auto mode = motionMode();
  if (mode == MM_ENABLED) state.setMotionMode(MM_DISABLED);
  if (mode == MM_DISABLED) state.setMotionMode(MM_ENABLED);
}

void Controls::syncThread() {
  if (!locked()) state.setThreadSyncState(motionMode() == MM_ENABLED ? SS_UNSYNC : SS_SYNC);
}

void Controls::jogToStop(LeadscrewStopPosition stop, JogGesture gesture) {
  if (locked()) return;
  const auto mode = motionMode();
  if (mode == MM_ENABLED) return;
  const bool left = stop == LeadscrewStopPosition::LEFT;
  const bool toggle = gesture == JogGesture::MatrixToggle;
  const auto requested = left ? MM_JOG_LEFT : MM_JOG_RIGHT;
  if (toggle && mode == requested) {
    state.setMotionMode(MM_DISABLED);
  } else if (screw.getStopPositionState(stop) != LeadscrewStopState::UNSET) {
    state.setMotionMode(requested);
    // Preserve the existing phase behavior of each control scheme.
    state.setThreadSyncState(left == toggle ? SS_SYNC : SS_UNSYNC);
  }
}

void Controls::toggleStop(LeadscrewStopPosition stop) {
  if (!canJog()) return;
  if (screw.getStopPositionState(stop) == LeadscrewStopState::UNSET) screw.setStopPosition(stop);
  else screw.unsetStopPosition(stop);
}

void Controls::cancelJog(GlobalMotionMode previousMode) {
  if (previousMode == MM_JOG_LEFT || previousMode == MM_JOG_RIGHT) state.setMotionMode(MM_DISABLED);
}

void Controls::requestDisplayReset() {
  if (!locked()) state.setDisplayReset();
}

void Controls::requestUpdate() {
  if (!locked()) state.setOTA();
}
