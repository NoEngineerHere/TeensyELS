#ifndef PIO_UNIT_TESTING
#define PIO_UNIT_TESTING
#endif

#include "arduino_test_mock.h"
#include <config.h>
#include <globalstate.h>
#include <gmock/gmock.h>

TEST(GlobalStateTest, SingletonInstance) {
  GlobalState* instance1 = GlobalState::getInstance();
  GlobalState* instance2 = GlobalState::getInstance();
  EXPECT_EQ(instance1, instance2);
}

TEST(GlobalStateTest, FeedModeManagement) {
  GlobalState* state = GlobalState::getInstance();
  
  state->setFeedMode(GlobalFeedMode::THREAD);
  EXPECT_EQ(state->getFeedMode(), GlobalFeedMode::THREAD);
  
  state->setFeedMode(GlobalFeedMode::FEED);
  EXPECT_EQ(state->getFeedMode(), GlobalFeedMode::FEED);
}

TEST(GlobalStateTest, MotionModeManagement) {
  GlobalState* state = GlobalState::getInstance();
  
  state->setMotionMode(GlobalMotionMode::MM_DISABLED);
  EXPECT_EQ(state->getMotionMode(), GlobalMotionMode::MM_DISABLED);
  
  state->setMotionMode(GlobalMotionMode::MM_ENABLED);
  EXPECT_EQ(state->getMotionMode(), GlobalMotionMode::MM_ENABLED);
  
  state->setMotionMode(GlobalMotionMode::JOG_LEFT);
  EXPECT_EQ(state->getMotionMode(), GlobalMotionMode::JOG_LEFT);
}

TEST(GlobalStateTest, ButtonLockManagement) {
  GlobalState* state = GlobalState::getInstance();
  
  state->setButtonLock(GlobalButtonLock::LOCKED);
  EXPECT_EQ(state->getButtonLock(), GlobalButtonLock::LOCKED);
  
  state->setButtonLock(GlobalButtonLock::UNLOCKED);
  EXPECT_EQ(state->getButtonLock(), GlobalButtonLock::UNLOCKED);
}

TEST(GlobalStateTest, UnitModeManagement) {
  GlobalState* state = GlobalState::getInstance();
  
  state->setUnitMode(GlobalUnitMode::METRIC);
  EXPECT_EQ(state->getUnitMode(), GlobalUnitMode::METRIC);
  
  state->setUnitMode(GlobalUnitMode::IMPERIAL);
  EXPECT_EQ(state->getUnitMode(), GlobalUnitMode::IMPERIAL);
}

TEST(GlobalStateTest, FeedSelectWithinBounds) {
  GlobalState* state = GlobalState::getInstance();
  
  // Test metric thread bounds
  state->setUnitMode(GlobalUnitMode::METRIC);
  state->setFeedMode(GlobalFeedMode::THREAD);
  int maxMetricThread = state->getCurrentFeedSelectArraySize() - 1;
  
  state->setFeedSelect(0);
  EXPECT_EQ(state->getFeedSelect(), 0);
  
  state->setFeedSelect(maxMetricThread);
  EXPECT_EQ(state->getFeedSelect(), maxMetricThread);
  
  // Test out of bounds gets clamped
  state->setFeedSelect(maxMetricThread + 10);
  EXPECT_EQ(state->getFeedSelect(), maxMetricThread);
}

TEST(GlobalStateTest, ThreadSyncStateManagement) {
  GlobalState* state = GlobalState::getInstance();
  
  state->setThreadSyncState(GlobalThreadSyncState::SYNC);
  EXPECT_EQ(state->getThreadSyncState(), GlobalThreadSyncState::SYNC);
  
  state->setThreadSyncState(GlobalThreadSyncState::UNSYNC);
  EXPECT_EQ(state->getThreadSyncState(), GlobalThreadSyncState::UNSYNC);
}

TEST(GlobalStateTest, CurrentFeedPitchCalculation) {
  GlobalState* state = GlobalState::getInstance();
  
  // Test metric thread pitch
  state->setUnitMode(GlobalUnitMode::METRIC);
  state->setFeedMode(GlobalFeedMode::THREAD);
  state->setFeedSelect(0);
  float pitch = state->getCurrentFeedPitch();
  EXPECT_GT(pitch, 0.0f);
  
  // Test feed pitch
  state->setFeedMode(GlobalFeedMode::FEED);
  state->setFeedSelect(0);
  float feedPitch = state->getCurrentFeedPitch();
  EXPECT_GT(feedPitch, 0.0f);
}