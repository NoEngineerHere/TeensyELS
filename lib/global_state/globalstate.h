
#include <axis.h>
#include <config.h>

#pragma once

#ifdef ELS_UI_ENCODER
enum class EncoderColour { NONE = 0, RED = 1, GREEN = 2, YELLOW = 3 };
#endif

// Major modes are the main modes of the application, like the feed or thread
// The spindle acts the same way in both threading and feeding mode
// this is just for the indicator on the screen
enum class GlobalFeedMode { UNSET = -1, FEED = 0, THREAD = 1 };

// The motion mode of the leadscrew in relation to the spindle
// Disabled: The leadscrew does not move when the spindle is moving
// Jog: The leadscrew is moving independently of the spindle
// Enabled: The leadscrew is moving in sync with the spindle
enum class GlobalMotionMode { UNSET, MM_DISABLED, JOG_LEFT, JOG_RIGHT, MM_ENABLED };

/**
 * The unit mode of the application, usually for threading
 * Choose either the superior metric system or the deprecated imperial system
 */
enum class GlobalUnitMode { METRIC, IMPERIAL };

/**
 * The state of the global thread sync
 * Sync: The spindle and leadscrew are in sync
 * Unsync: The spindle and leadscrew are out of sync
 */
enum class GlobalThreadSyncState { UNSET, SYNC, UNSYNC };

/**
 * The state of the global button lock
 * Unlocked: The buttons are unlocked
 * Locked: The buttons are locked
 */
enum class GlobalButtonLock { UNSET, UNLOCKED, LOCKED };


// this is a singleton class - we don't want more than one of these existing at
// a time!
class GlobalState {
 private:
  static GlobalState *m_instance;
  bool OTA = false;
  int OTAbytes = 0;
  int OTAlength = 0;

  GlobalFeedMode m_feedMode;
  GlobalMotionMode m_motionMode;
  GlobalUnitMode m_unitMode;
  GlobalThreadSyncState m_threadSyncState;
  GlobalButtonLock m_buttonLock;

  bool m_debugMode = false;
  bool m_displayReset = false;

  int m_feedSelect;

  // the position at which the spindle will be back in sync with the leadscrew
  // note that this position actually has *two* solutions, left and right
  // but we only use the "left" position and calculate the "right" position when
  // required
  int m_resyncPulseCount;

  GlobalState() {
    setFeedMode(DEFAULT_FEED_MODE);
    setUnitMode(DEFAULT_UNIT_MODE);
    setButtonLock(GlobalButtonLock::LOCKED);
    setFeedSelect(-1);
    setThreadSyncState(GlobalThreadSyncState::UNSYNC);
    m_motionMode = GlobalMotionMode::MM_DISABLED;
    m_resyncPulseCount = 0;
  }

 public:

  // singleton stuff, no cloning and no copying
  GlobalState(GlobalState const &) = delete;
  void operator=(GlobalState const &) = delete;

  static GlobalState *getInstance();

  void setFeedMode(GlobalFeedMode mode);
  GlobalFeedMode getFeedMode();

  void setMotionMode(GlobalMotionMode mode);
  GlobalMotionMode getMotionMode();

  void setUnitMode(GlobalUnitMode mode);
  GlobalUnitMode getUnitMode();

  void setThreadSyncState(GlobalThreadSyncState state);
  GlobalThreadSyncState getThreadSyncState();

  void setButtonLock(GlobalButtonLock lock);
  GlobalButtonLock getButtonLock();

  bool hasOTA();
  void setOTA();
  void clearOTA();

  void setOTABytes(int bytes);
  int getOTABytes();
  int getOTALength();
  void setOTAContentLength(int length);

  void setDisplayReset();
  bool getDisplayReset();

  void setFeedSelect(int select);
  int getFeedSelect();
  float getCurrentFeedPitch();
  int nextFeedPitch();
  int prevFeedPitch();

  int getCurrentFeedSelectArraySize();
};
