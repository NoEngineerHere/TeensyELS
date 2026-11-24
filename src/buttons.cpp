#include "buttons.h"

#include <config.h>

ButtonHandler::ButtonHandler(Controls& controls)
  : controls(controls),
  m_rateIncrease(ELS_RATE_INCREASE_BUTTON),
  m_rateDecrease(ELS_RATE_DECREASE_BUTTON),
  m_modeCycle(ELS_MODE_CYCLE_BUTTON),
  m_threadSync(ELS_THREAD_SYNC_BUTTON),
  m_enable(ELS_ENABLE_BUTTON),
  m_lock(ELS_LOCK_BUTTON),
  m_jogLeft(ELS_JOG_LEFT_BUTTON),
  m_jogRight(ELS_JOG_RIGHT_BUTTON) {}

void ButtonHandler::handle() {
  // update the state of the application based on the button state
  rateIncreaseHandler();
  rateDecreaseHandler();
  modeCycleHandler();
  threadSyncHandler();
  enableHandler();
  lockHandler();
  jogHandler();
}

void ButtonHandler::rateIncreaseHandler() {
  m_rateIncrease.handle();

  if (controls.locked()) {
    m_rateIncrease.resetClicked();
    m_rateIncrease.resetSingleClicked();
    m_rateIncrease.resetDoubleClicked();

    return;
  }

  if (m_rateIncrease.resetSingleClicked()) {
    controls.changePitch(1);
  }
}

void ButtonHandler::rateDecreaseHandler() {
  m_rateDecrease.handle();

  if (controls.locked()) {
    m_rateDecrease.resetClicked();
    m_rateDecrease.resetSingleClicked();
    m_rateDecrease.resetDoubleClicked();
    return;
  }

  if (m_rateDecrease.resetSingleClicked()) {
    controls.changePitch(-1);
  }
}

void ButtonHandler::enableHandler() {
  m_enable.handle();

  if (controls.locked()) {
    m_enable.resetClicked();
    m_enable.resetSingleClicked();
    m_enable.resetDoubleClicked();
    return;
  }

  if (m_enable.resetClicked()) controls.toggleMotion();
}

void ButtonHandler::lockHandler() {
  m_lock.handle();

  if (m_lock.resetClicked()) controls.toggleLock();
}

void ButtonHandler::threadSyncHandler() {
  m_threadSync.handle();

  if (controls.locked()) {
    m_threadSync.resetClicked();
    m_threadSync.resetSingleClicked();
    m_threadSync.resetDoubleClicked();
    return;
  }

  if (m_threadSync.resetClicked()) controls.syncThread();
}

void ButtonHandler::modeCycleHandler() {
  m_modeCycle.handle();

  if (controls.locked()) {
    m_modeHoldHandled = m_modeCycle.isPressed();
    m_modeCycle.resetClicked();
    m_modeCycle.resetSingleClicked();
    m_modeCycle.resetDoubleClicked();
    return;
  }

  // pressing mode button swaps between feed and thread
  if (m_modeCycle.resetClicked() && !m_modeHoldHandled) {
    controls.toggleFeedMode();
  }
  if (!m_modeCycle.isPressed()) m_modeHoldHandled = false;

  // holding mode button swaps between metric and imperial
  if (m_modeCycle.isHeld() && !m_modeHoldHandled) {
    m_modeHoldHandled = true;
    controls.toggleUnits();
  }
}

void ButtonHandler::jogDirectionHandler(JogDirection direction) {
  const auto stop = direction == LEFT ? LeadscrewStopPosition::LEFT : LeadscrewStopPosition::RIGHT;
  Button* jogButton =
    direction == JogDirection::LEFT ? &m_jogLeft : &m_jogRight;
  bool& heldHandled = direction == JogDirection::LEFT ? m_leftHoldHandled : m_rightHoldHandled;

  // no jogging functionality allowed during lock or enable
  if (!controls.canJog()) {
    heldHandled = jogButton->isPressed();
    jogButton->resetClicked();
    jogButton->resetSingleClicked();
    jogButton->resetDoubleClicked();
    return;
  }

  // single click should jog to the stop position
  if (jogButton->resetClicked() && !heldHandled) {
    controls.jogToStop(stop, Controls::JogGesture::DirectClick);
  }
  if (!jogButton->isPressed()) heldHandled = false;

  /**
   * holding the jog button will set/unset the stop position
   */
  if (jogButton->isHeld() && !heldHandled) {
    heldHandled = true;
    controls.toggleStop(stop);
  }
}

void ButtonHandler::jogHandler() {
  const auto motionMode = controls.motionMode();
  m_jogLeft.handle();
  m_jogRight.handle();

  jogDirectionHandler(JogDirection::LEFT);
  jogDirectionHandler(JogDirection::RIGHT);

  // common jog functionality
  // if neither jog button is held, reset the motion mode
  if (!m_jogLeft.isHeld() && !m_jogRight.isHeld()) controls.cancelJog(motionMode);
}
