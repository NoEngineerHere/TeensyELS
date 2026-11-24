#pragma once
#include <globalstate.h>
#include <leadscrew.h>
#include <mk2_panel.h>

struct DisplayStatus {
  GlobalFeedMode feed;
  GlobalMotionMode motion;
  GlobalButtonLock lock;
  GlobalThreadSyncState sync;
  bool leftStop, rightStop;
  char pitch[10];
  char rpm[8];
  char position[21];
};

// The smallest display fits 21 characters at the built-in font's 6 px advance.
using DisplayLines = char[7][22];
DisplayStatus readDisplayStatus(GlobalState& state, Spindle& spindle, Leadscrew& screw);
void formatDisplayLines(DisplayLines& lines, const DisplayStatus& status, const Mk2Panel* panel);
bool formatUpdateLines(DisplayLines& lines, const UpdateStatus& status);
