#include "legacy_panel.h"

namespace {
enum LegacyKey { Decrease, Increase, Mode, Sync, HalfNut, Enable, Lock, Left, Right };
}

void LegacyPanel::key(MatrixEvent event) {
  if (event.gesture == MatrixGesture::Click) {
    switch (event.key) {
      case Increase: controls.changePitch(1); break;
      case Decrease: controls.changePitch(-1); break;
      case Mode: controls.toggleFeedMode(); break;
      case Sync: controls.syncThread(); break;
      case Enable: controls.toggleMotion(); break;
      case Lock: controls.toggleLock(); break;
      case Left: controls.jogToStop(LeadscrewStopPosition::LEFT, Controls::JogGesture::MatrixToggle); break;
      case Right: controls.jogToStop(LeadscrewStopPosition::RIGHT, Controls::JogGesture::MatrixToggle); break;
    }
  } else if (event.gesture == MatrixGesture::Hold) {
    switch (event.key) {
      case Mode: controls.toggleUnits(); break;
      case Sync: controls.requestDisplayReset(); break;
      case HalfNut: if (supportsUpdate) controls.requestUpdate(); break;
      case Left: controls.toggleStop(LeadscrewStopPosition::LEFT); break;
      case Right: controls.toggleStop(LeadscrewStopPosition::RIGHT); break;
    }
  }
}
