#include <gtest/gtest.h>
#include <clock.h>
#include <globalstate.h>
#include <leadscrew.h>
#include "mocks/leadscrewio_mock.h"

class PositionTest : public testing::Test {
protected:
  void SetUp() override {
    setTestMicros(0);
    auto* state = GlobalState::getInstance();
    state->setUnitMode(DEFAULT_UNIT_MODE);
    state->setFeedMode(DEFAULT_FEED_MODE);
    state->setFeedSelect(-1);
    state->setMotionMode(MM_DISABLED);
    state->setThreadSyncState(SS_UNSYNC);
    state->setButtonLock(LK_LOCKED);
    state->clearOTA();
  }
};

TEST_F(PositionTest, SignedDeltaIsConsumedExactlyOnce) {
  Spindle spindle;
  spindle.incrementCurrentPosition(321);
  spindle.incrementCurrentPosition(-100);
  EXPECT_EQ(spindle.consumePosition(), 221);
  EXPECT_EQ(spindle.consumePosition(), 0);
  spindle.incrementCurrentPosition(-500);
  EXPECT_EQ(spindle.consumePosition(), -500);
  EXPECT_EQ(spindle.consumePosition(), 0);
}

TEST_F(PositionTest, DisabledDiscardsEncoderMovement) {
  Spindle spindle;
  LeadscrewIOMock io;
  Leadscrew screw(&spindle, &io, 1000, 100, 800, 2.54, 1200);
  ASSERT_TRUE(screw.initializeOutputs());
  screw.setFaultInhibit(false);
  for (uint32_t t = 4; t <= 20000; t += 4) {
    setTestMicros(t);
    spindle.incrementCurrentPosition(-1);
    screw.update();
  }
  EXPECT_EQ(screw.getCurrentPosition(), 0);
  EXPECT_EQ(screw.getPositionError(), 0);
  EXPECT_EQ(io.readStepPin(), 0);
  EXPECT_EQ(spindle.consumePosition(), 0);
}

TEST_F(PositionTest, PitchProducesSignedCompletedPulseDisplacement) {
  for (int direction : {-1, 1}) {
    setTestMicros(0);
    GlobalState::getInstance()->setMotionMode(MM_DISABLED);
    Spindle spindle;
    LeadscrewIOMock io;
    Leadscrew screw(&spindle, &io, 1000, 100, 800, 2.54, 1200);
    ASSERT_TRUE(screw.initializeOutputs());
    screw.setFaultInhibit(false);
    screw.setTargetPitchMM(1.27); // 400 completed steps per spindle revolution.
    GlobalState::getInstance()->setMotionMode(MM_ENABLED);
    screw.update(); // Accept explicit re-arm before introducing fresh spindle movement.
    spindle.incrementCurrentPosition(direction * 1200);
    for (uint32_t t = 4; t <= 100000; t += 4) {
      setTestMicros(t);
      screw.update();
    }
    EXPECT_EQ(screw.getCurrentPosition(), direction * 400);
    EXPECT_EQ(io.readStepPin(), 0);
    EXPECT_EQ(io.readDirPin(), direction > 0 ? ELS_DIR_RIGHT : ELS_DIR_LEFT);
  }
}
