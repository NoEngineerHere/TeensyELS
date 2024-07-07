
/**
 * this holds global state for the ELS, e.g: current mode, estop etc
 */
class ELS_State {
  enum DriveState { FEED, THREAD };

  bool m_locked = true;  // default to safe state
  bool m_enabled = false;
  bool m_readyToThread = false;
  DriveState m_driveState = FEED;

  // getters
  bool getLockState();
  bool getEnabledState();
  bool getReadyToThreadState();
  DriveState getDriveState();

  // setters, also returns the new state
  bool setLockState(bool newLockState);
  bool setEnabledState(bool newEnabledState);
  bool setReadyToThreadState(bool newReadyToThreadState);
  DriveState setDriveState(DriveState newDriveState);
};