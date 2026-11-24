#include <gtest/gtest.h>
#include <spindle.h>
#include <leadscrew.h>
#include <globalstate.h>
#include <atomic>
#include <thread>
#include <vector>
#include <climits>
#include "mocks/leadscrewio_mock.h"

TEST(Timing, SpindleWindowAcrossWrapAndBothDirections) {
  for (int sign : {-1, 1}) {
    setTestMicros(0xfffffff0u);
    Spindle spindle;
    EXPECT_EQ(spindle.getEstimatedVelocityInRPM(), 0);
    setTestMicros(0x10);
    spindle.incrementCurrentPosition(sign * 600);
    EXPECT_FLOAT_EQ(spindle.getEstimatedVelocityInPPS(), 18750000);
    EXPECT_GE(spindle.getCurrentPosition(), 0);
    EXPECT_LT(spindle.getCurrentPosition(), ELS_SPINDLE_ENCODER_PPR);
    spindle.consumePosition();
    setTestMicros(0x10 + 500000);
    spindle.incrementCurrentPosition(sign * 600);
    EXPECT_FLOAT_EQ(spindle.getEstimatedVelocityInPPS(), 1200);
    EXPECT_FLOAT_EQ(spindle.getEstimatedVelocityInRPM(), 60);
  }
}

TEST(Timing, StaleVelocityStaysInvalidThroughTwoWraps) {
  setTestMicros(0);
  Spindle spindle;
  setTestMicros(500000);
  spindle.incrementCurrentPosition(600);
  EXPECT_FLOAT_EQ(spindle.getEstimatedVelocityInRPM(), 60);
  spindle.consumePosition();
  for (uint64_t t = 1500001; t < (uint64_t(2) << 32) + 1500001; t += 500000) {
    setTestMicros(static_cast<uint32_t>(t));
    spindle.update();
    ASSERT_EQ(spindle.getEstimatedVelocityInRPM(), 0);
  }
  const uint32_t now = elsMicros();
  spindle.incrementCurrentPosition(100);
  EXPECT_EQ(spindle.getEstimatedVelocityInRPM(), 0);
  setTestMicros(now + 500000);
  spindle.incrementCurrentPosition(500);
  EXPECT_FLOAT_EQ(spindle.getEstimatedVelocityInRPM(), 60);
}

TEST(Timing, CountOverflowLatchesFaultWithoutChangingKnownPosition) {
  setTestMicros(0);
  Spindle spindle;
  spindle.incrementCurrentPosition(INT_MAX);
  const int known = spindle.getCurrentPosition();
  spindle.incrementCurrentPosition(1);
  EXPECT_TRUE(spindle.hasMotionFault());
  EXPECT_EQ(spindle.getCurrentPosition(), known);
  EXPECT_EQ(spindle.consumePosition(), INT_MAX);
  spindle.incrementCurrentPosition(-100);
  EXPECT_EQ(spindle.consumePosition(), 0);
  EXPECT_EQ(spindle.getEstimatedVelocityInRPM(), 0);
}

TEST(Timing, VelocityReadersObserveWholePublishedSamples) {
  setTestMicros(0);
  Spindle spindle;
  std::atomic<bool> done{false};
  std::thread writer([&] {
    uint32_t now = 0;
    for (int i = 0; i < 20000; ++i) {
      now += i % 2 ? 250000 : 500000;
      setTestMicros(now);
      spindle.incrementCurrentPosition(i % 2 ? -900 : 600);
      spindle.consumePosition();
    }
    done = true;
  });
  bool coherent = true;
  do {
    const float rpm = spindle.getEstimatedVelocityInRPM();
    if (rpm != 0 && rpm != 60 && rpm != 180) coherent = false;
  } while (!done);
  writer.join();
  EXPECT_TRUE(coherent);
}

static std::vector<int> pulseTrace(uint32_t base, GlobalMotionMode mode) {
  setTestMicros(base);
  auto* state = GlobalState::getInstance();
  state->setUnitMode(DEFAULT_UNIT_MODE);
  state->setFeedMode(DEFAULT_FEED_MODE);
  state->setThreadSyncState(SS_UNSYNC);
  state->setMotionMode(MM_DISABLED);
  Spindle spindle;
  LeadscrewIOMock io;
  Leadscrew screw(&spindle, &io, 1000, 100, 800, 2.54, 1200);
  EXPECT_TRUE(screw.initializeOutputs());
  screw.setFaultInhibit(false);
  screw.setTargetPitchMM(1.27);
  state->setMotionMode(mode);
  screw.update();
  spindle.incrementCurrentPosition(1200);
  std::vector<int> trace;
  for (uint32_t t = 4; t <= 20000; t += 4) {
    setTestMicros(base + t);
    spindle.update();
    screw.update();
    trace.push_back(screw.getCurrentPosition());
    trace.push_back(io.readStepPin());
    trace.push_back(io.readDirPin());
  }
  return trace;
}
TEST(Timing, EnabledAndJogTracesAreInvariantAtClockWrap) {
  for (auto mode : {MM_ENABLED, MM_JOG_LEFT, MM_JOG_RIGHT, MM_DISABLED})
    EXPECT_EQ(pulseTrace(0, mode), pulseTrace(0xfffffff0u, mode));
}
