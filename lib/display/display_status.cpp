#include "display_status.h"
#include "rpm_format.h"
#include <cstdio>

DisplayStatus readDisplayStatus(GlobalState& state, Spindle& spindle, Leadscrew& screw) {
  DisplayStatus status{state.getFeedMode(), state.getMotionMode(), state.getButtonLock(),
    state.getThreadSyncState(),
    screw.getStopPositionState(LeadscrewStopPosition::LEFT) == LeadscrewStopState::SET,
    screw.getStopPositionState(LeadscrewStopPosition::RIGHT) == LeadscrewStopState::SET, {}, {}};
  const auto index = state.getFeedSelect();
  const bool thread = status.feed == FM_THREAD;
  if (state.getUnitMode() == METRIC) {
    std::snprintf(status.pitch, sizeof(status.pitch), "%.2fmm",
        thread ? threadPitchMetric[index] : feedPitchMetric[index]);
  } else {
    std::snprintf(status.pitch, sizeof(status.pitch), thread ? "%dTPI" : "%dth",
        int(thread ? threadPitchImperial[index] : feedPitchImperial[index] * 1000));
  }
  formatRpm(spindle.getEstimatedVelocityInRPM(), status.rpm);
  return status;
}

void formatDisplayLines(DisplayLines& lines, const DisplayStatus& s, const Mk2Panel* panel) {
  const auto sel = panel ? panel->selection() : Mk2Selection{};
  const bool locked = s.lock == LK_LOCKED;
  const char* motion = s.motion == MM_ENABLED ? "Enabled" :
    s.motion == MM_JOG_LEFT || s.motion == MM_JOG_RIGHT ? "Jogging" : "Disabled";
  std::snprintf(lines[0], sizeof(lines[0]), "%s", !panel ? "TeensyELS" :
      sel.view == Mk2View::Menu ? "Menu" : sel.view == Mk2View::Stops ? "Stops" : "Rate");
  std::snprintf(lines[1], sizeof(lines[1]), "%s %s", s.feed == FM_THREAD ? "Thread" : "Feed", s.pitch);
  std::snprintf(lines[2], sizeof(lines[2]), "%s %s", motion, locked ? "Locked" : "Unlocked");
  std::snprintf(lines[3], sizeof(lines[3]), "%s %s %c%c", s.rpm,
      s.sync == SS_SYNC ? "Sync" : "Unsynced", s.leftStop ? '[' : '-', s.rightStop ? ']' : '-');
  if (sel.view == Mk2View::Menu) {
    const char* entries[] = {locked ? "Unlock" : "Lock", "Units", "Sync", "Reset display", "Update"};
    const auto n = sel.menuCount;
    for (unsigned i=0; i<3; ++i)
      std::snprintf(lines[4+i], sizeof(lines[4+i]), "%c %s",
          i == 1 ? '>' : ' ', entries[(sel.menu + n + i - 1) % n]);
  } else if (sel.view == Mk2View::Stops) {
    std::snprintf(lines[4], sizeof(lines[4]), "Selected: %s",
        sel.stop == LeadscrewStopPosition::LEFT ? "left" : "right");
  }
}
