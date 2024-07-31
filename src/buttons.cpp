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
  Button* keys[] = {&m_rateIncrease, &m_rateDecrease, &m_modeCycle,
                    &m_threadSync,   &m_halfNut,      &m_enable,
                    &m_lock,         &m_jogLeft,      &m_jogRight};

  ButtonList keyPad(keys);
  m_keyPad = &keyPad;
  Button setHeldTime(100);
}

void ButtonHandler::handle() {
  // update the button state
  m_keyPad->handle();
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
  Serial.println("Rate Inc Call");
  GlobalButtonLock lockState = GlobalState::getInstance()->getButtonLock();

  if (lockState == GlobalButtonLock::LOCKED) {
    return;
  }

  if (m_rateIncrease.isClicked()) {
    GlobalState::getInstance()->nextFeedPitch();
    m_leadscrew->setRatio(GlobalState::getInstance()->getCurrentFeedPitch());
  }
}

void ButtonHandler::rateDecreaseHandler() {
  Serial.println("Rate Dec Call");

  GlobalButtonLock lockState = GlobalState::getInstance()->getButtonLock();
  if (lockState == GlobalButtonLock::LOCKED) {
    return;
  }

  if (m_rateDecrease.isClicked()) {
    GlobalState::getInstance()->prevFeedPitch();
    m_leadscrew->setRatio(GlobalState::getInstance()->getCurrentFeedPitch());
  }
}

void ButtonHandler::halfNutHandler() {
  Serial.println("Half Nut Call");

  GlobalButtonLock lockState = GlobalState::getInstance()->getButtonLock();
  if (lockState == GlobalButtonLock::LOCKED) {
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
  Serial.println("Enable Call");

  GlobalButtonLock lockState = GlobalState::getInstance()->getButtonLock();
  if (lockState == GlobalButtonLock::LOCKED) {
    return;
  }

  if (m_enable.isClicked()) {
    if (GlobalState::getInstance()->getMotionMode() ==
        GlobalMotionMode::ENABLED) {
      GlobalState::getInstance()->setMotionMode(GlobalMotionMode::DISABLED);
    } else {
      GlobalState::getInstance()->setMotionMode(GlobalMotionMode::ENABLED);
    }
  }
}

void ButtonHandler::lockHandler() {
  Serial.println("Lock Call");

  GlobalState* globalState = GlobalState::getInstance();

  if (m_lock.isClicked()) {
    if (globalState->getButtonLock() == GlobalButtonLock::LOCKED) {
      globalState->setButtonLock(GlobalButtonLock::UNLOCKED);
    } else {
      globalState->setButtonLock(GlobalButtonLock::LOCKED);
    }
  }
}

void ButtonHandler::threadSyncHandler() {
  Serial.println("Thread Sync Call");

  GlobalButtonLock lockState = GlobalState::getInstance()->getButtonLock();
  if (lockState == GlobalButtonLock::LOCKED) {
    return;
  }

  if (m_threadSync.isClicked()) {
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
  Serial.print("Mode Cycle Call");
  GlobalState* globalState = GlobalState::getInstance();
  GlobalButtonLock lockState = globalState->getButtonLock();

  if (lockState == GlobalButtonLock::LOCKED) {
    return;
  }

  // pressing mode button swaps between feed and thread
  if (m_modeCycle.isClicked()) {
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

  // no jogging functionality allowed during lock or enable
  if (lockState == GlobalButtonLock::LOCKED ||
      motionMode == GlobalMotionMode::ENABLED) {
    return;
  }

  if (m_jogLeft.isDoubleClicked()) {
    // todo set the leadscrew stop position
  }

  Button* jogButton =
      direction == JogDirection::LEFT ? &m_jogLeft : &m_jogRight;
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
  Serial.println("Jog Left Call");
  jogHandler(JogDirection::LEFT);
}

void ButtonHandler::jogRightHandler() {
  Serial.println("Jog Right Call");

  jogHandler(JogDirection::RIGHT);
}
