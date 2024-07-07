#include "state.h"

bool ELS_State::getLockState() { return this->m_locked; }
bool ELS_State::setLockState(bool newLockState) {
  // this is the only setting that doesn't check the lock state
  this->m_locked = newLockState;
  return this->m_locked;
}

bool ELS_State::getEnabledState() { return this->m_enabled; };
bool ELS_State::setEnabledState(bool newEnabledState) {
  if (!this->m_locked) {
    this->m_enabled = newEnabledState;
  }
  return this->m_enabled;
}

ELS_State::DriveState ELS_State::getDriveState() { return this->m_driveState; };
ELS_State::DriveState ELS_State::setDriveState(
    ELS_State::DriveState newDriveState) {
  if (!this->m_locked) {
    this->m_driveState = newDriveState;
  }
  return this->m_driveState;
}

bool ELS_State::getReadyToThreadState() { return this->m_readyToThread; }
bool ELS_State::setReadyToThreadState(bool newReadyToThread) {
  // previously this didn't check if we were locked before setting, do we want
  // to keep that?
  if (!this->m_locked) {
    this->m_readyToThread = newReadyToThread;
  }
  return this->m_readyToThread;
}
