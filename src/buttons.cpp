#include "buttons.h"

#include <config.h>
#include <globalstate.h>

ButtonHandler::ButtonHandler(Spindle* spindle, Leadscrew* leadscrew)
    : m_spindle(spindle),
      m_leadscrew(leadscrew),
      m_rateIncrease(ELS_RATE_INCREASE_BUTTON),
      m_rateDecrease(ELS_RATE_DECREASE_BUTTON),
      m_modeCycle(ELS_MODE_CYCLE_BUTTON),
      m_threadSync(ELS_THREAD_SYNC_BUTTON),
      m_halfNut(ELS_HALF_NUT_BUTTON),
      m_enable(ELS_ENABLE_BUTTON),
      m_lock(ELS_LOCK_BUTTON),
      m_jogLeft(ELS_JOG_LEFT_BUTTON),
      m_jogRight(ELS_JOG_RIGHT_BUTTON) {
  Button setHeldTime(50);
  Button setClickTime(200);
}

void ButtonHandler::handle() {
  // update the state of the application based on the button state
  rateIncreaseHandler();
  rateDecreaseHandler();
  modeCycleHandler();
  threadSyncHandler();
  halfNutHandler();
  enableHandler();
  lockHandler();
  jogHandler();
}

void ButtonHandler::rateIncreaseHandler() {
  m_rateIncrease.handle();

  GlobalButtonLock lockState = GlobalState::getInstance()->getButtonLock();
  if (lockState == GlobalButtonLock::LOCKED) {
    m_rateIncrease.resetClicked();
    m_rateIncrease.resetSingleClicked();
    m_rateIncrease.resetDoubleClicked();

    return;
  }

  if (m_rateIncrease.resetSingleClicked()) {
    GlobalState::getInstance()->nextFeedPitch();
    m_leadscrew->setRatio(GlobalState::getInstance()->getCurrentFeedPitch());
  }
}

void ButtonHandler::printState() {
  Serial.print("Enable: ");
  if (m_enable.isHeld()) {
    Serial.println("held");
  } else if (m_enable.isPressed()) {
    Serial.println("pressed");
  } else if (m_enable.isDoubleClicked()) {
    Serial.println("double clicked");
  } else {
    Serial.println("released");
  }
  Serial.print("Left jog: ");
  if (m_jogLeft.isHeld()) {
    Serial.println("held");
  } else if (m_jogLeft.isPressed()) {
    Serial.println("pressed");
  } else if (m_jogLeft.isDoubleClicked()) {
    Serial.println("double clicked");
  } else {
    Serial.println("released");
  }
  Serial.print("Right jog: ");
  if (m_jogRight.isHeld()) {
    Serial.println("held");
  } else if (m_jogRight.isPressed()) {
    Serial.println("pressed");
  } else if (m_jogRight.isDoubleClicked()) {
    Serial.println("double clicked");
  } else {
    Serial.println("released");
  }
}

void ButtonHandler::rateDecreaseHandler() {
  m_rateDecrease.handle();

  GlobalButtonLock lockState = GlobalState::getInstance()->getButtonLock();
  if (lockState == GlobalButtonLock::LOCKED) {
    m_rateDecrease.resetClicked();
    m_rateDecrease.resetSingleClicked();
    m_rateDecrease.resetDoubleClicked();
    return;
  }

  if (m_rateDecrease.resetSingleClicked()) {
    GlobalState::getInstance()->prevFeedPitch();
    m_leadscrew->setRatio(GlobalState::getInstance()->getCurrentFeedPitch());
  }
}

void ButtonHandler::halfNutHandler() {
  m_halfNut.handle();

  GlobalButtonLock lockState = GlobalState::getInstance()->getButtonLock();
  if (lockState == GlobalButtonLock::LOCKED) {
    m_halfNut.resetClicked();
    m_halfNut.resetSingleClicked();
    m_halfNut.resetDoubleClicked();
    return;
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

void ButtonHandler::enableHandler() {
  m_enable.handle();

  GlobalButtonLock lockState = GlobalState::getInstance()->getButtonLock();
  GlobalMotionMode motionMode = GlobalState::getInstance()->getMotionMode();
  if (lockState == GlobalButtonLock::LOCKED) {
    m_enable.resetClicked();
    m_enable.resetSingleClicked();
    m_enable.resetDoubleClicked();
    return;
  }

  if (m_enable.resetClicked()) {
    Serial.println("Enable button clicked");
    if (motionMode == GlobalMotionMode::ENABLED) {
      GlobalState::getInstance()->setMotionMode(GlobalMotionMode::DISABLED);
    }
    if (motionMode == GlobalMotionMode::DISABLED) {
      GlobalState::getInstance()->setMotionMode(GlobalMotionMode::ENABLED);
    }
  }
}

void ButtonHandler::lockHandler() {
  m_lock.handle();

  GlobalState* globalState = GlobalState::getInstance();

  if (m_lock.resetClicked()) {
    if (globalState->getButtonLock() == GlobalButtonLock::LOCKED) {
      globalState->setButtonLock(GlobalButtonLock::UNLOCKED);
    } else {
      globalState->setButtonLock(GlobalButtonLock::LOCKED);
    }
  }
}

void ButtonHandler::threadSyncHandler() {
  m_threadSync.handle();

  GlobalButtonLock lockState = GlobalState::getInstance()->getButtonLock();
  if (lockState == GlobalButtonLock::LOCKED) {
    m_threadSync.resetClicked();
    m_threadSync.resetSingleClicked();
    m_threadSync.resetDoubleClicked();
    return;
  }

  if (m_threadSync.resetClicked()) {
    if (GlobalState::getInstance()->getMotionMode() ==
        GlobalMotionMode::ENABLED) {
      GlobalState::getInstance()->setThreadSyncState(
          GlobalThreadSyncState::UNSYNC);
    } else {
      GlobalState::getInstance()->setThreadSyncState(
          GlobalThreadSyncState::SYNC);
    }
  }
}

void ButtonHandler::modeCycleHandler() {
  m_modeCycle.handle();

  GlobalState* globalState = GlobalState::getInstance();
  GlobalButtonLock lockState = globalState->getButtonLock();

  if (lockState == GlobalButtonLock::LOCKED) {
    m_modeCycle.resetClicked();
    m_modeCycle.resetSingleClicked();
    m_modeCycle.resetDoubleClicked();
    return;
  }

  // pressing mode button swaps between feed and thread
  if (m_modeCycle.resetClicked()) {
    switch (GlobalState::getInstance()->getFeedMode()) {
      case GlobalFeedMode::FEED:
        GlobalState::getInstance()->setFeedMode(GlobalFeedMode::THREAD);
        break;
      case GlobalFeedMode::THREAD:
        GlobalState::getInstance()->setFeedMode(GlobalFeedMode::FEED);
        break;
    }
    m_leadscrew->setRatio(globalState->getCurrentFeedPitch());
  }

  // holding mode button swaps between metric and imperial
  if (m_modeCycle.isHeld()) {
    switch (GlobalState::getInstance()->getUnitMode()) {
      case GlobalUnitMode::METRIC:
        GlobalState::getInstance()->setUnitMode(GlobalUnitMode::IMPERIAL);
        break;
      case GlobalUnitMode::IMPERIAL:
        GlobalState::getInstance()->setUnitMode(GlobalUnitMode::METRIC);
        break;
    }
    m_leadscrew->setRatio(globalState->getCurrentFeedPitch());
  }
}

void ButtonHandler::jogDirectionHandler(JogDirection direction) {
  GlobalState* globalState = GlobalState::getInstance();
  GlobalButtonLock lockState = globalState->getButtonLock();
  GlobalMotionMode motionMode = globalState->getMotionMode();

  Button* jogButton =
      direction == JogDirection::LEFT ? &m_jogLeft : &m_jogRight;

  // no jogging functionality allowed during lock or enable
  if (lockState == GlobalButtonLock::LOCKED ||
      motionMode == GlobalMotionMode::ENABLED) {
    jogButton->resetClicked();
    jogButton->resetSingleClicked();
    jogButton->resetDoubleClicked();
    return;
  }

  /*// single click should jog to the stop position
  if(jogButton->resetClicked()) {
   switch(direction) {
     case JogDirection::LEFT:
      if(m_leadscrew->getStopPositionState(Leadscrew::StopPosition::LEFT) != LeadscrewStopState::UNSET) {
          m_leadscrew->setExpectedPosition(m_leadscrew->getStopPosition(Leadscrew::StopPosition::LEFT));
      }
      break;
     case JogDirection::RIGHT:
      if(m_leadscrew->getStopPositionState(Leadscrew::StopPosition::RIGHT) != LeadscrewStopState::UNSET) {
          m_leadscrew->setExpectedPosition(m_leadscrew->getStopPosition(Leadscrew::StopPosition::RIGHT));
      }
      break;
   }
  }*/

  if (jogButton->isDoubleClicked()) {
    switch (direction) {
      case JogDirection::LEFT:
        if (m_leadscrew->getStopPositionState(Leadscrew::StopPosition::LEFT) ==
            LeadscrewStopState::UNSET) {
          m_leadscrew->setStopPosition(Leadscrew::StopPosition::LEFT,
                                       m_leadscrew->getCurrentPosition());
        } else {
          m_leadscrew->unsetStopPosition(Leadscrew::StopPosition::LEFT);
        }
        break;
      case JogDirection::RIGHT:
        if (m_leadscrew->getStopPositionState(Leadscrew::StopPosition::RIGHT) ==
            LeadscrewStopState::UNSET) {
          m_leadscrew->setStopPosition(Leadscrew::StopPosition::RIGHT,
                                       m_leadscrew->getCurrentPosition());
        } else {
          m_leadscrew->unsetStopPosition(Leadscrew::StopPosition::RIGHT);
        }
        break;
    }
    jogButton->resetDoubleClicked();
  }

  static elapsedMicros jogTimer;

  if (jogButton->isHeld() &&
      jogTimer > JOG_PULSE_DELAY) {
    globalState->setMotionMode(GlobalMotionMode::JOG);
    globalState->setThreadSyncState(GlobalThreadSyncState::UNSYNC);

    jogTimer -= JOG_PULSE_DELAY;
    m_leadscrew->setExpectedPosition(
        m_leadscrew->getCurrentPosition() + (int)direction);
  }
}

void ButtonHandler::jogHandler() {
  GlobalMotionMode motionMode = GlobalState::getInstance()->getMotionMode();
  m_jogLeft.handle();
  m_jogRight.handle();

  jogDirectionHandler(JogDirection::LEFT);
  jogDirectionHandler(JogDirection::RIGHT);

  // common jog functionality
  // if neither jog button is held, reset the motion mode
  if (!m_jogLeft.isHeld() && !m_jogRight.isHeld() &&
      motionMode == GlobalMotionMode::JOG) {
    GlobalState::getInstance()->setMotionMode(GlobalMotionMode::DISABLED);
  }
}
