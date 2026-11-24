#pragma once

#include <globalstate.h>
#include <leadscrew.h>

class Controls {
 public:
  Controls(GlobalState& state, Leadscrew& screw) : state(state), screw(screw) {}

  bool locked() { return state.getButtonLock() == LK_LOCKED; }
  GlobalMotionMode motionMode() { return state.getMotionMode(); }
  bool canJog() { return !locked() && motionMode() != MM_ENABLED; }

  void changePitch(int64_t increments);
  void toggleFeedMode();
  void toggleUnits();
  void toggleLock();
  void toggleMotion();
  void stopMotion();
  void syncThread();
  enum class JogGesture { DirectClick, MatrixToggle };
  void jogToStop(LeadscrewStopPosition stop, JogGesture gesture);
  void toggleStop(LeadscrewStopPosition stop);
  void cancelJog(GlobalMotionMode previousMode);
  void requestDisplayReset();
  void requestUpdate();

 private:
  GlobalState& state;
  Leadscrew& screw;
};
