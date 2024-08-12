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
  Button setHeldTime(100);
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
  jogLeftHandler();
  jogRightHandler();
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
  if (lockState == GlobalButtonLock::LOCKED) {
    m_enable.resetClicked();
    m_enable.resetSingleClicked();
    m_enable.resetDoubleClicked();
    return;
  }

  if (m_enable.resetClicked()) {
    if (GlobalState::getInstance()->getMotionMode() ==
        GlobalMotionMode::ENABLED) {
      GlobalState::getInstance()->setMotionMode(GlobalMotionMode::DISABLED);
    } else {
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
  }

  // holding mode button swaps between metric and imperial
  static boolean heldHandled = false;  // keep track of whether or not we have
                                       // handled the "held" event yet
  if (m_modeCycle.isHeld()) {
    heldHandled = true;
    switch (GlobalState::getInstance()->getUnitMode()) {
      case GlobalUnitMode::METRIC:
        GlobalState::getInstance()->setUnitMode(GlobalUnitMode::IMPERIAL);
        break;
      case GlobalUnitMode::IMPERIAL:
        GlobalState::getInstance()->setUnitMode(GlobalUnitMode::METRIC);
        break;
    }
  } else {
    heldHandled = false;
  }

  m_leadscrew->setRatio(globalState->getCurrentFeedPitch());
}

void ButtonHandler::jogHandler(JogDirection direction) {
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

  if (jogButton->isDoubleClicked()) {
    switch (direction) {
      case JogDirection::LEFT:
        m_leadscrew->setStopPosition(Leadscrew::StopPosition::LEFT,
                                     m_leadscrew->getCurrentPosition());
        break;
      case JogDirection::RIGHT:
        m_leadscrew->setStopPosition(Leadscrew::StopPosition::RIGHT,
                                     m_leadscrew->getCurrentPosition());

        break;
    }
    // todo set the leadscrew stop position
  }

  static elapsedMicros jogTimer;

  if (jogButton->isHeld() && jogTimer > JOG_PULSE_DELAY_US) {
    jogTimer -= JOG_PULSE_DELAY_US;
    globalState->setMotionMode(GlobalMotionMode::JOG);
    globalState->setThreadSyncState(GlobalThreadSyncState::UNSYNC);
    m_leadscrew->incrementCurrentPosition(-1);
  } else if (!jogButton->isPressed()) {
    globalState->setMotionMode(GlobalMotionMode::DISABLED);
  }
}

void ButtonHandler::jogLeftHandler() {
  m_jogLeft.handle();

  jogHandler(JogDirection::LEFT);
}

void ButtonHandler::jogRightHandler() {
  m_jogRight.handle();

  jogHandler(JogDirection::RIGHT);
}
