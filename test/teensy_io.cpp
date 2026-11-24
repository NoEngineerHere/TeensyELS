#include <gtest/gtest.h>
#include <vector>
#define ELS_TEST_TEENSY_IO
#define ELS_LEADSCREW_STEP 2
#define ELS_LEADSCREW_DIR 3
constexpr int LOW=0,HIGH=1,OUTPUT=1;
struct PinEvent {int pin,value;bool output;};
static std::vector<PinEvent> events;
static void digitalWriteFast(int pin,int value){events.push_back({pin,value,false});}
static void pinMode(int pin,int value){events.push_back({pin,value,true});}
#include <pulse_teensy.h>
TEST(TeensyIO, StartsLowAndCompletesWrappedHighPhaseOnlyOnce) {
 events.clear();LeadscrewIOTeensy<2, 3> io({4, 4, 4, 4});ASSERT_TRUE(io.begin());
 ASSERT_GE(events.size(),2u);EXPECT_EQ(events[0].pin,ELS_LEADSCREW_STEP);
 EXPECT_FALSE(events[0].output);EXPECT_EQ(events[0].value,LOW);
 EXPECT_EQ(events[1].pin,ELS_LEADSCREW_STEP);EXPECT_TRUE(events[1].output);
 EXPECT_EQ(io.servicePulse(0).result,LeadscrewIO::PulseResult::IDLE);
 setTestMicros(0xfffffffeu);ASSERT_TRUE(io.startPulse(0xfffffffeu));EXPECT_FALSE(io.startPulse(0xffffffffu));
 const auto highEvents=events.size();
 EXPECT_EQ(io.servicePulse(1).result,LeadscrewIO::PulseResult::PENDING);EXPECT_EQ(events.size(),highEvents);
 setTestMicros(3);
 const auto completed = io.servicePulse(2);
 EXPECT_EQ(completed.result,LeadscrewIO::PulseResult::COMPLETE);
 EXPECT_EQ(completed.lowSince,3u);
 EXPECT_EQ(events.back().value,LOW);const auto completedEvents=events.size();
 EXPECT_EQ(io.servicePulse(6).result,LeadscrewIO::PulseResult::IDLE);EXPECT_EQ(events.size(),completedEvents);
}
