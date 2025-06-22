#include <config.h>

#ifdef ELS_USE_BUTTON_ARRAY
#include "buttonpad.h"

#include <config.h>
#include <globalstate.h>
#include <HttpsOTAUpdate.h>
#include "SECRETS.h"

ButtonPad::ButtonPad(Spindle* spindle, Leadscrew* leadscrew, KeyArray* pad)
  : m_spindle(spindle),
  m_leadscrew(leadscrew),
  m_pad(pad) {
}

void ButtonPad::handle() {

  ButtonInfo press = m_pad->consumeButton();
  switch (press.button) {
  case ELS_RATE_INCREASE_BUTTON:
    rateIncreaseHandler(press);
    break;
  case ELS_RATE_DECREASE_BUTTON:
    rateDecreaseHandler(press);
    break;
  case ELS_MODE_CYCLE_BUTTON:
    modeCycleHandler(press);
    break;
  case ELS_THREAD_SYNC_BUTTON:
    threadSyncHandler(press);
    break;
  case ELS_HALF_NUT_BUTTON:
    halfNutHandler(press);
    break;
  case ELS_ENABLE_BUTTON:
    enableHandler(press);
    break;
  case ELS_LOCK_BUTTON:
    lockHandler(press);
    break;
  case ELS_JOG_LEFT_BUTTON:
  case ELS_JOG_RIGHT_BUTTON:
    jogHandler(press);
    break;
  default:
    break;
  }
}

void ButtonPad::rateIncreaseHandler(ButtonInfo press) {

  GlobalButtonLock lockState = GlobalState::getInstance()->getButtonLock();
  if (lockState == GlobalButtonLock::LK_LOCKED) {
    return;
  }

  if (press.buttonState == BS_CLICKED) {
    GlobalState::getInstance()->nextFeedPitch();
    m_leadscrew->setTargetPitchMM(GlobalState::getInstance()->getCurrentFeedPitch());
  }
}

void ButtonPad::rateDecreaseHandler(ButtonInfo press) {

  GlobalButtonLock lockState = GlobalState::getInstance()->getButtonLock();
  if (lockState == GlobalButtonLock::LK_LOCKED) {
    return;
  }

  if (press.buttonState == BS_CLICKED) {
    GlobalState::getInstance()->prevFeedPitch();
    m_leadscrew->setTargetPitchMM(GlobalState::getInstance()->getCurrentFeedPitch());
  }
}

void ButtonPad::halfNutHandler(ButtonInfo press) {

  GlobalButtonLock lockState = GlobalState::getInstance()->getButtonLock();
  if (lockState == GlobalButtonLock::LK_LOCKED) {
    return;
  }

  if (press.buttonState == BS_HELD) {
    GlobalState::getInstance()->setOTA();
  }

  // honestly I don't know what this button should do after the refactor...

  /*if (event == Button::SINGLE_CLICKED_EVENT &&
      globalState->getFeedMode() == GlobalFeedMode::THREAD) {
    readyToThread = true;
    globalState->setMotionMode(GlobalMotionMode::ENABLED);
    globalState->setThreadSyncState(GlobalThreadSyncState::SYNC);
    pulsesBackToSync = 0;
  }*/
}

void ButtonPad::enableHandler(ButtonInfo press) {
  GlobalButtonLock lockState = GlobalState::getInstance()->getButtonLock();
  GlobalMotionMode motionMode = GlobalState::getInstance()->getMotionMode();
  if (lockState == GlobalButtonLock::LK_LOCKED) {
    return;
  }

  if (press.buttonState == BS_CLICKED) {
    if (motionMode == GlobalMotionMode::MM_ENABLED) {
      GlobalState::getInstance()->setMotionMode(GlobalMotionMode::MM_DISABLED);
    }
    if (motionMode == GlobalMotionMode::MM_DISABLED) {
      GlobalState::getInstance()->setMotionMode(GlobalMotionMode::MM_ENABLED);
    }
  }
}

void ButtonPad::lockHandler(ButtonInfo press) {
  GlobalState* globalState = GlobalState::getInstance();

  if (press.buttonState == BS_CLICKED) {
    if (globalState->getButtonLock() == GlobalButtonLock::LK_LOCKED) {
      globalState->setButtonLock(GlobalButtonLock::LK_UNLOCKED);
    } else {
      globalState->setButtonLock(GlobalButtonLock::LK_LOCKED);
    }
  }
}

void ButtonPad::threadSyncHandler(ButtonInfo press) {
  GlobalButtonLock lockState = GlobalState::getInstance()->getButtonLock();
  if (lockState == GlobalButtonLock::LK_LOCKED) {
    return;
  }
  if (press.buttonState == BS_HELD) {
    GlobalState::getInstance()->setDisplayReset();
  }
  if (press.buttonState == BS_CLICKED) {
    if (GlobalState::getInstance()->getMotionMode() ==
      GlobalMotionMode::MM_ENABLED) {
      GlobalState::getInstance()->setThreadSyncState(
        GlobalThreadSyncState::SS_UNSYNC);
    } else {
      GlobalState::getInstance()->setThreadSyncState(
        GlobalThreadSyncState::SS_SYNC);
    }
  }
}

void ButtonPad::modeCycleHandler(ButtonInfo press) {

  GlobalState* globalState = GlobalState::getInstance();
  GlobalButtonLock lockState = globalState->getButtonLock();

  if (lockState == GlobalButtonLock::LK_LOCKED) {
    return;
  }

  // pressing mode button swaps between feed and thread
  if (press.buttonState == BS_CLICKED) {
    switch (GlobalState::getInstance()->getFeedMode()) {
    case GlobalFeedMode::FM_FEED:
      GlobalState::getInstance()->setFeedMode(GlobalFeedMode::FM_THREAD);
      break;
    case GlobalFeedMode::FM_THREAD:
      GlobalState::getInstance()->setFeedMode(GlobalFeedMode::FM_FEED);
      break;
    }
    m_leadscrew->setTargetPitchMM(globalState->getCurrentFeedPitch());
  }

  // holding mode button swaps between metric and imperial
  if (press.buttonState == BS_HELD) {
    switch (GlobalState::getInstance()->getUnitMode()) {
    case GlobalUnitMode::METRIC:
      GlobalState::getInstance()->setUnitMode(GlobalUnitMode::IMPERIAL);
      break;
    case GlobalUnitMode::IMPERIAL:
      GlobalState::getInstance()->setUnitMode(GlobalUnitMode::METRIC);
      break;
    }
    m_leadscrew->setTargetPitchMM(globalState->getCurrentFeedPitch());
  }
}

void ButtonPad::jogDirectionHandler(ButtonInfo press) {
  GlobalState* globalState = GlobalState::getInstance();
  GlobalButtonLock lockState = globalState->getButtonLock();
  GlobalMotionMode motionMode = globalState->getMotionMode();

  // no jogging functionality allowed during lock or enable
  if (lockState == GlobalButtonLock::LK_LOCKED ||
    motionMode == GlobalMotionMode::MM_ENABLED) {
    return;
  }

  // single click should jog to the stop position
  if (press.buttonState == BS_CLICKED) {
    switch (press.button) {
    case ELS_JOG_LEFT_BUTTON:
      if (globalState->getMotionMode() == GlobalMotionMode::MM_JOG_LEFT) {
        globalState->setMotionMode(GlobalMotionMode::MM_DISABLED);
        break;
      }
      if (m_leadscrew->getStopPositionState(LeadscrewStopPosition::LEFT) != LeadscrewStopState::UNSET) {
        globalState->setMotionMode(GlobalMotionMode::MM_JOG_LEFT);
        globalState->setThreadSyncState(GlobalThreadSyncState::SS_SYNC);
      }
      break;
    case ELS_JOG_RIGHT_BUTTON:
      if (globalState->getMotionMode() == GlobalMotionMode::MM_JOG_RIGHT) {
        globalState->setMotionMode(GlobalMotionMode::MM_DISABLED);
        break;
      }
      if (m_leadscrew->getStopPositionState(LeadscrewStopPosition::RIGHT) != LeadscrewStopState::UNSET) {
        globalState->setMotionMode(GlobalMotionMode::MM_JOG_RIGHT);
        globalState->setThreadSyncState(GlobalThreadSyncState::SS_UNSYNC);
      }
      break;
    }
  }

  /**
   * holding the jog button will set/unset the stop position
   */
  if (press.buttonState == BS_HELD) {
    switch (press.button) {
    case ELS_JOG_LEFT_BUTTON:
      if (m_leadscrew->getStopPositionState(LeadscrewStopPosition::LEFT) ==
        LeadscrewStopState::UNSET) {
        m_leadscrew->setStopPosition(LeadscrewStopPosition::LEFT);

      } else {
        m_leadscrew->unsetStopPosition(LeadscrewStopPosition::LEFT);
      }
      break;
    case ELS_JOG_RIGHT_BUTTON:
      if (m_leadscrew->getStopPositionState(LeadscrewStopPosition::RIGHT) ==
        LeadscrewStopState::UNSET) {
        m_leadscrew->setStopPosition(LeadscrewStopPosition::RIGHT);
      } else {
        m_leadscrew->unsetStopPosition(LeadscrewStopPosition::RIGHT);
      }
      break;
    }
  }
}

void ButtonPad::jogHandler(ButtonInfo press) {
  GlobalMotionMode motionMode = GlobalState::getInstance()->getMotionMode();

  jogDirectionHandler(press);

  // common jog functionality
  // if neither jog button is held, reset the motion mode
//  if (press.buttonState == BS_RELEASED &&
//    motionMode == GlobalMotionMode::MM_JOG) {
//    GlobalState::getInstance()->setMotionMode(GlobalMotionMode::MM_DISABLED);
//  }
}


#endif