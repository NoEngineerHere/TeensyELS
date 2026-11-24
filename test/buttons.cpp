#include <gtest/gtest.h>
#include <new>
#include "mocks/leadscrewio_mock.h"
#include "../src/buttons.cpp"

class Buttons : public testing::Test {
protected:
 Spindle spindle;
 LeadscrewIOMock io;
 Leadscrew screw{&spindle,&io,1000,100,1200,2,1200};
 // Production ButtonHandler has static storage. Mirror its zero-initialized
 // backing for AbleButtons' internal counters; exercise its real pin state machine.
 alignas(ButtonHandler) unsigned char storage[sizeof(ButtonHandler)]{};
 ButtonHandler* handler=nullptr;
 void SetUp() override {
  setTestMicros(1000000);ButtonHardwareTest::pins.fill(HIGH);
  auto* s=GlobalState::getInstance();s->setUnitMode(METRIC);s->setFeedMode(FM_FEED);
  s->setButtonLock(LK_UNLOCKED);s->setMotionMode(MM_DISABLED);s->setThreadSyncState(SS_UNSYNC);
  handler=new(storage) ButtonHandler(&spindle,&screw);
  poll(60);
 }
 void TearDown() override {handler->~ButtonHandler();}
 void poll(unsigned ms,ButtonHandler* target=nullptr) {
  for(unsigned i=0;i<ms;i+=10){setTestMicros(elsMicros()+10000);(target?target:handler)->handle();}
 }
 void press(int pin){ButtonHardwareTest::pins[pin]=LOW;handler->handle();}
 void release(int pin){ButtonHardwareTest::pins[pin]=HIGH;handler->handle();poll(50);}
};
TEST_F(Buttons, ModeHoldActsOnceAndReleaseDoesNotAlsoClick) {
 press(ELS_MODE_CYCLE_BUTTON);poll(1100);
 EXPECT_EQ(GlobalState::getInstance()->getUnitMode(),IMPERIAL);
 poll(1100);EXPECT_EQ(GlobalState::getInstance()->getUnitMode(),IMPERIAL);
 release(ELS_MODE_CYCLE_BUTTON);EXPECT_EQ(GlobalState::getInstance()->getFeedMode(),FM_FEED);
 press(ELS_MODE_CYCLE_BUTTON);poll(1100);
 EXPECT_EQ(GlobalState::getInstance()->getUnitMode(),METRIC);
}
TEST_F(Buttons, ShortModeClickChangesFeedNotUnits) {
 press(ELS_MODE_CYCLE_BUTTON);poll(150);release(ELS_MODE_CYCLE_BUTTON);
 EXPECT_EQ(GlobalState::getInstance()->getFeedMode(),FM_THREAD);
 EXPECT_EQ(GlobalState::getInstance()->getUnitMode(),METRIC);
}
TEST_F(Buttons, LockedHoldIsConsumedUntilDebouncedRelease) {
 auto* s=GlobalState::getInstance();s->setButtonLock(LK_LOCKED);
 press(ELS_MODE_CYCLE_BUTTON);poll(1200);s->setButtonLock(LK_UNLOCKED);poll(1200);
 EXPECT_EQ(s->getUnitMode(),METRIC);release(ELS_MODE_CYCLE_BUTTON);
 EXPECT_EQ(s->getFeedMode(),FM_FEED);
 press(ELS_MODE_CYCLE_BUTTON);poll(1100);EXPECT_EQ(s->getUnitMode(),IMPERIAL);
}
TEST_F(Buttons, JogStopsHaveIndependentOneShotHoldsAndNoReleaseJog) {
 press(ELS_JOG_LEFT_BUTTON);poll(1100);
 EXPECT_EQ(screw.getStopPositionState(LeadscrewStopPosition::LEFT),LeadscrewStopState::SET);
 poll(1100);EXPECT_EQ(screw.getStopPositionState(LeadscrewStopPosition::LEFT),LeadscrewStopState::SET);
 press(ELS_JOG_RIGHT_BUTTON);poll(1100);
 EXPECT_EQ(screw.getStopPositionState(LeadscrewStopPosition::LEFT),LeadscrewStopState::SET);
 EXPECT_EQ(screw.getStopPositionState(LeadscrewStopPosition::RIGHT),LeadscrewStopState::SET);
 release(ELS_JOG_LEFT_BUTTON);EXPECT_EQ(GlobalState::getInstance()->getMotionMode(),MM_DISABLED);
 press(ELS_JOG_LEFT_BUTTON);poll(1100);
 EXPECT_EQ(screw.getStopPositionState(LeadscrewStopPosition::LEFT),LeadscrewStopState::UNSET);
 EXPECT_EQ(screw.getStopPositionState(LeadscrewStopPosition::RIGHT),LeadscrewStopState::SET);
}
TEST_F(Buttons, HoldLatchesAreNotSharedBetweenHandlerInstances) {
 press(ELS_MODE_CYCLE_BUTTON);poll(1100);ASSERT_EQ(GlobalState::getInstance()->getUnitMode(),IMPERIAL);
 alignas(ButtonHandler) unsigned char secondStorage[sizeof(ButtonHandler)]{};
 auto* second=new(secondStorage) ButtonHandler(&spindle,&screw);
 second->handle();poll(1100,second);EXPECT_EQ(GlobalState::getInstance()->getUnitMode(),METRIC);
 handler->handle();EXPECT_EQ(GlobalState::getInstance()->getUnitMode(),METRIC);
 second->~ButtonHandler();
}
TEST_F(Buttons, HeldLeftJogIsNotUnconditionallyCancelledByBooleanPrecedence) {
 press(ELS_JOG_LEFT_BUTTON);poll(1100);
 GlobalState::getInstance()->setMotionMode(MM_JOG_LEFT);handler->handle();
 EXPECT_EQ(GlobalState::getInstance()->getMotionMode(),MM_JOG_LEFT);
 release(ELS_JOG_LEFT_BUTTON);EXPECT_EQ(GlobalState::getInstance()->getMotionMode(),MM_DISABLED);
}
