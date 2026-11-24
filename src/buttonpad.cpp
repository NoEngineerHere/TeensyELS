#include <els/hardware_config.h>

#if ELS_INPUT_LAYOUT == ELS_INPUT_LEGACY_MATRIX
#include "buttonpad.h"

void ButtonPad::handle() {
  if constexpr (hardware.encoder.a >= 0) {
    const int64_t delta = pad.consumeEncoderDelta();
    if (delta != 0) controls.changePitch(delta);
  }
  const ButtonInfo press = pad.consumeButton();
  if (press.buttonState == BS_CLICKED) {
    switch (press.button) {
      case 17: controls.changePitch(1); break;
      case 9: controls.changePitch(-1); break;
      case 33: controls.toggleFeedMode(); break;
      case 10: controls.syncThread(); break;
      case 34: controls.toggleMotion(); break;
      case 12: controls.toggleLock(); break;
      case 20: controls.jogToStop(LeadscrewStopPosition::LEFT, Controls::JogGesture::MatrixToggle); break;
      case 36: controls.jogToStop(LeadscrewStopPosition::RIGHT, Controls::JogGesture::MatrixToggle); break;
    }
  } else if (press.buttonState == BS_HELD) {
    switch (press.button) {
      case 33: controls.toggleUnits(); break;
      case 10: controls.requestDisplayReset(); break;
      case 18: controls.requestUpdate(); break;
      case 20: controls.toggleStop(LeadscrewStopPosition::LEFT); break;
      case 36: controls.toggleStop(LeadscrewStopPosition::RIGHT); break;
    }
  }
}
#endif