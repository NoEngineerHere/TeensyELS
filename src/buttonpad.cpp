#include <config.h>

#ifdef ELS_USE_BUTTON_ARRAY
#include "buttonpad.h"

void ButtonPad::handle() {
#ifdef ELS_UI_ENCODER
  const int64_t delta = pad.consumeEncoderDelta();
  if (delta != 0) controls.changePitch(delta);
#endif
  const ButtonInfo press = pad.consumeButton();
  if (press.buttonState == BS_CLICKED) {
    switch (press.button) {
      case ELS_RATE_INCREASE_BUTTON: controls.changePitch(1); break;
      case ELS_RATE_DECREASE_BUTTON: controls.changePitch(-1); break;
      case ELS_MODE_CYCLE_BUTTON: controls.toggleFeedMode(); break;
      case ELS_THREAD_SYNC_BUTTON: controls.syncThread(); break;
      case ELS_ENABLE_BUTTON: controls.toggleMotion(); break;
      case ELS_LOCK_BUTTON: controls.toggleLock(); break;
      case ELS_JOG_LEFT_BUTTON: controls.jogToStop(LeadscrewStopPosition::LEFT, Controls::JogGesture::MatrixToggle); break;
      case ELS_JOG_RIGHT_BUTTON: controls.jogToStop(LeadscrewStopPosition::RIGHT, Controls::JogGesture::MatrixToggle); break;
    }
  } else if (press.buttonState == BS_HELD) {
    switch (press.button) {
      case ELS_MODE_CYCLE_BUTTON: controls.toggleUnits(); break;
      case ELS_THREAD_SYNC_BUTTON: controls.requestDisplayReset(); break;
      case ELS_HALF_NUT_BUTTON: controls.requestUpdate(); break;
      case ELS_JOG_LEFT_BUTTON: controls.toggleStop(LeadscrewStopPosition::LEFT); break;
      case ELS_JOG_RIGHT_BUTTON: controls.toggleStop(LeadscrewStopPosition::RIGHT); break;
    }
  }
}
#endif