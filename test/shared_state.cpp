#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <globalstate.h>
#include "mocks/leadscrewio_mock.h"
#include <leadscrew.h>

namespace {
struct StartupDefaults {GlobalUnitMode unit;GlobalFeedMode feed;GlobalMotionMode motion;GlobalButtonLock lock;GlobalThreadSyncState sync;int selection;float pitch;};
// Capture the real singleton's initialized state before tests mutate it. The
// production constructor remains private; no alternate constructor model.
const StartupDefaults startup=[] {
 auto* s=GlobalState::getInstance();
 return StartupDefaults{s->getUnitMode(),s->getFeedMode(),s->getMotionMode(),s->getButtonLock(),s->getThreadSyncState(),s->getFeedSelect(),s->getCurrentFeedPitch()};
}();
}
TEST(SharedState, InitializedStartupSelectsValidPitchBeforeControls) {
 EXPECT_EQ(startup.unit,DEFAULT_UNIT_MODE);EXPECT_EQ(startup.feed,DEFAULT_FEED_MODE);
 EXPECT_EQ(startup.motion,MM_DISABLED);EXPECT_EQ(startup.lock,LK_LOCKED);EXPECT_EQ(startup.sync,SS_UNSYNC);
 const int expected=DEFAULT_UNIT_MODE==METRIC?
  (DEFAULT_FEED_MODE==FM_THREAD?DEFAULT_METRIC_THREAD_PITCH_IDX:DEFAULT_METRIC_FEED_PITCH_IDX):
  (DEFAULT_FEED_MODE==FM_THREAD?DEFAULT_IMPERIAL_THREAD_PITCH_IDX:DEFAULT_IMPERIAL_FEED_PITCH_IDX);
 EXPECT_EQ(startup.selection,expected);
 const float pitch=DEFAULT_UNIT_MODE==METRIC?
  (DEFAULT_FEED_MODE==FM_THREAD?threadPitchMetric[expected]:feedPitchMetric[expected]):
  (DEFAULT_FEED_MODE==FM_THREAD?25.4f/threadPitchImperial[expected]:feedPitchImperial[expected]*25.4f/1000);
 EXPECT_FLOAT_EQ(startup.pitch,pitch);
}
TEST(SharedState, ConcurrentSpindleConsumeConservesEveryEncoderIncrement) {
 setTestMicros(1000000);Spindle spindle;std::atomic<bool> done{false};
 constexpr int counts=40000;
 std::thread producer([&]{for(int i=0;i<counts;++i)spindle.incrementCurrentPosition(1);done.store(true);});
 int64_t consumed=0;
 while(!done.load()){consumed+=spindle.consumePosition();std::this_thread::yield();}
 producer.join();consumed+=spindle.consumePosition();
 EXPECT_EQ(consumed,counts);EXPECT_EQ(spindle.consumePosition(),0);
 EXPECT_EQ(spindle.getCurrentPosition(),counts%ELS_SPINDLE_ENCODER_PPR);EXPECT_FALSE(spindle.hasMotionFault());
}
TEST(SharedState, PositionReplacementAndErrorReadObserveOneCoherentPair) {
 setTestMicros(1000000);Spindle spindle;LeadscrewIOMock io;Leadscrew screw(&spindle,&io,1000,100,1200,2,1200);
 std::atomic<bool> done{false};bool mixed=false;
 std::thread writer([&]{for(int i=0;i<20000;++i)screw.setCurrentPosition(i%2?100000:-100000);done.store(true);});
 while(!done.load()){mixed|=screw.getPositionError()!=0;std::this_thread::yield();}
 writer.join();EXPECT_FALSE(mixed);EXPECT_EQ(screw.getPositionError(),0);
}
TEST(SharedState, MotionTransitionResetIsConsumedOnceWithoutLosingMode) {
 auto* s=GlobalState::getInstance();bool reset=false;
 s->setMotionMode(MM_DISABLED);s->takeMotionRequest(reset);
 s->setMotionMode(MM_ENABLED);EXPECT_EQ(s->takeMotionRequest(reset),MM_ENABLED);EXPECT_TRUE(reset);
 EXPECT_EQ(s->takeMotionRequest(reset),MM_ENABLED);EXPECT_FALSE(reset);
 s->setMotionMode(MM_ENABLED);EXPECT_EQ(s->takeMotionRequest(reset),MM_ENABLED);EXPECT_FALSE(reset);
 s->setMotionMode(MM_DISABLED);EXPECT_EQ(s->takeMotionRequest(reset),MM_DISABLED);EXPECT_TRUE(reset);
}
