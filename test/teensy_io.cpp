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
#include <leadscrew_io_teensy.h>
TEST(TeensyIO, StartsLowAndCompletesWrappedHighPhaseOnlyOnce) {
 events.clear();LeadscrewIOTeensy io;ASSERT_TRUE(io.begin());
 ASSERT_GE(events.size(),2u);EXPECT_EQ(events[0].pin,ELS_LEADSCREW_STEP);
 EXPECT_FALSE(events[0].output);EXPECT_EQ(events[0].value,LOW);
 EXPECT_EQ(events[1].pin,ELS_LEADSCREW_STEP);EXPECT_TRUE(events[1].output);
 EXPECT_EQ(io.servicePulse(0),LeadscrewIO::PulseResult::IDLE);
 ASSERT_TRUE(io.startPulse(0xfffffffeu));EXPECT_FALSE(io.startPulse(0xffffffffu));
 const auto highEvents=events.size();
 EXPECT_EQ(io.servicePulse(1),LeadscrewIO::PulseResult::PENDING);EXPECT_EQ(events.size(),highEvents);
 EXPECT_EQ(io.servicePulse(2),LeadscrewIO::PulseResult::COMPLETE);
 EXPECT_EQ(events.back().value,LOW);const auto completedEvents=events.size();
 EXPECT_EQ(io.servicePulse(6),LeadscrewIO::PulseResult::IDLE);EXPECT_EQ(events.size(),completedEvents);
}
