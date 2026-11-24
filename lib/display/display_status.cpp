#include "display_status.h"
#include "rpm_format.h"
#include <cstdio>

DisplayStatus readDisplayStatus(GlobalState& state, Spindle& spindle, Leadscrew& screw) {
  const auto selection = state.feedSelection();
  DisplayStatus status{selection.mode, state.getMotionMode(), state.getButtonLock(),
    state.getThreadSyncState(),
    screw.getStopPositionState(LeadscrewStopPosition::LEFT) == LeadscrewStopState::SET,
    screw.getStopPositionState(LeadscrewStopPosition::RIGHT) == LeadscrewStopState::SET, {}, {}, {}};
  const auto index = selection.index;
  const bool thread = isThreadMode(status.feed);
  if (selection.unit == METRIC) {
    std::snprintf(status.pitch, sizeof(status.pitch), "%.2fmm",
        thread ? threadPitchMetric[index] : feedPitchMetric[index]);
  } else {
    std::snprintf(status.pitch, sizeof(status.pitch), thread ? "%dTPI" : "%dth",
        int(thread ? threadPitchImperial[index] : feedPitchImperial[index] * 1000));
  }
  formatRpm(spindle.getEstimatedVelocityInRPM(), status.rpm);
  const auto position = screw.positionReading();
  if (!position.valid) {
    std::snprintf(status.position, sizeof(status.position), "Position unknown");
  } else {
    using Datum = Leadscrew::PositionDatum;
    const char* label = position.datum == Datum::Left ? "Left" : position.datum == Datum::Right ? "Right" : "Origin";
    const bool metric = selection.unit == METRIC;
    const double value = metric ? position.millimeters : position.millimeters / 25.4;
    const char* unit = metric ? "mm" : "in";
    const int length = std::snprintf(status.position, sizeof(status.position), "%s %+.3f%s", label, value, unit);
    if (length >= int(sizeof(status.position)))
      std::snprintf(status.position, sizeof(status.position), "%s %+.4g%s", label, value, unit);
  }
  return status;
}

void formatDisplayLines(DisplayLines& lines, const DisplayStatus& s, const Mk2Panel* panel) {
  const auto sel = panel ? panel->selection() : Mk2Selection{};
  const bool locked = s.lock == LK_LOCKED;
  const char* motion = s.motion == MM_ENABLED ? "Enabled" :
    s.motion == MM_JOG_LEFT || s.motion == MM_JOG_RIGHT ? "Jogging" : "Disabled";
  std::snprintf(lines[0], sizeof(lines[0]), "%s", !panel ? "TeensyELS" :
      sel.view == Mk2View::Menu ? "Menu" : sel.view == Mk2View::Stops ? "Stops" : "Rate");
  std::snprintf(lines[1], sizeof(lines[1]), "%s %s", s.feed == FM_THREAD_REVERSE ? "Reverse" : s.feed == FM_THREAD ? "Thread" : "Feed", s.pitch);
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
    std::snprintf(lines[5], sizeof(lines[5]), "%s", s.position);
  } else {
    std::snprintf(lines[4], sizeof(lines[4]), "%s", s.position);
  }
}

bool formatUpdateLines(DisplayLines& lines, const UpdateStatus& status) {
  switch (status.phase) {
    case UpdatePhase::Idle: return false;
    case UpdatePhase::Draining:
      std::snprintf(lines[0], sizeof(lines[0]), "Update");
      std::snprintf(lines[1], sizeof(lines[1]), "Waiting for idle");
      break;
    case UpdatePhase::Waiting:
      std::snprintf(lines[0], sizeof(lines[0]), "Join %s", status.network);
      std::snprintf(lines[1], sizeof(lines[1]), "Wi-Fi key:");
      std::snprintf(lines[2], sizeof(lines[2]), "%.16s", status.key);
      std::snprintf(lines[3], sizeof(lines[3]), "%.16s", status.key+16);
      std::snprintf(lines[4], sizeof(lines[4]), "IP %s", status.address);
      std::snprintf(lines[5], sizeof(lines[5]), "PUT /firmware");
      break;
    case UpdatePhase::Receiving:
      std::snprintf(lines[0], sizeof(lines[0]), "Updating");
      if (status.total > 0)
        std::snprintf(lines[1], sizeof(lines[1]), "%lu/%lu",
            static_cast<unsigned long>(status.received), static_cast<unsigned long>(status.total));
      break;
    case UpdatePhase::Failed:
      std::snprintf(lines[0], sizeof(lines[0]), "Update failed");
      std::snprintf(lines[1], sizeof(lines[1]), "%s", status.detail);
      std::snprintf(lines[2], sizeof(lines[2]), "Reboot locally");
      break;
    case UpdatePhase::Complete:
      std::snprintf(lines[0], sizeof(lines[0]), "Update verified");
      break;
  }
  return true;
}
