
#include "axis/axis.h"
#include "axis/leadscrew/leadscrew.h"
#include "axis/spindle/spindle.h"
#include "config.h"

#pragma once

// Major modes are the main modes of the application, like the feed or thread
// The spindle acts the same way in both threading and feeding mode
// this is just for the indicator on the screen
enum GlobalFeedMode { FEED, THREAD };

// The motion mode of the leadscrew in relation to the spindle
// Disabled: The leadscrew does not move when the spindle is moving
// Jog: The leadscrew is moving independently of the spindle
// Enabled: The leadscrew is moving in sync with the spindle
enum GlobalMotionMode { DISABLED, JOG, ENABLED };

/**
 * The unit mode of the application, usually for threading
 * Choose either the superior metric system or the deprecated imperial system
 */
enum GlobalUnitMode { METRIC, IMPERIAL };

// this is a singleton class - we don't want more than one of these existing at
// a time!
class GlobalState {
 private:
  static GlobalState *m_instance;

  // todo jogging better

  GlobalFeedMode m_feedMode;
  GlobalMotionMode m_minorMode;
  GlobalUnitMode m_unitMode;

  int m_feedSelect;

  // pulse count for the "stop" positions of the spindle
  int m_stop1Position;
  int m_stop2Position;

  // the position at which the spindle will be back in sync with the leadscrew
  // note that this position actually has *two* solutions, left and right
  // but we only use the "left" position and calculate the "right" position when
  // required
  int m_resyncPulseCount;

  GlobalState() {
    m_feedSelect = 0;
    setFeedMode(DEFAULT_FEED_MODE);
    setUnitMode(DEFAULT_UNIT_MODE);
    m_minorMode = DISABLED;
    m_stop1Position = 0;
    m_stop2Position = 0;
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

  void setFeedSelect(int select);
  int getFeedSelect();
  float getCurrentFeedPitch();
  int nextFeedPitch();
  int prevFeedPitch();

  void setStopPosition(int position, int pulseCount);
  int getStopPosition(int position);

 protected:
  int getCurrentFeedSelectArraySize();
};
