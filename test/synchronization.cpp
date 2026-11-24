#include <gtest/gtest.h>
#include <leadscrew.h>
#include <globalstate.h>
#include <limits>
#include <new>
#include "mocks/leadscrewio_mock.h"

class Synchronization : public testing::Test {
protected:
 void SetUp() override {
  setTestMicros(0);
  auto* s=GlobalState::getInstance();
  s->setUnitMode(METRIC); s->setFeedMode(FM_FEED); s->setFeedSelect(-1);
  s->setMotionMode(MM_DISABLED); s->setThreadSyncState(SS_UNSYNC); s->setButtonLock(LK_LOCKED);
 }
 void expectPhase(Leadscrew& screw, Spindle& spindle, int position, int phase) {
  screw.setCurrentPosition(position);
  spindle.setCurrentPosition(phase); spindle.consumePosition();
  GlobalState::getInstance()->setMotionMode(MM_ENABLED);
  screw.update();
  GlobalState::getInstance()->setThreadSyncState(SS_UNSYNC);
  setTestMicros(elsMicros()+1000); screw.update();
  EXPECT_EQ(GlobalState::getInstance()->getThreadSyncState(),SS_SYNC);
  EXPECT_EQ(screw.getCurrentPosition(),position);
 }
};
TEST_F(Synchronization, FreshAutomaticAndZeroStorageRemainUnregistered) {
 Spindle spindle; LeadscrewIOMock io;
 Leadscrew automatic(&spindle,&io,1000,100,1200,2,1200);
 alignas(Leadscrew) unsigned char storage[sizeof(Leadscrew)]{};
 auto* zeroed=new(storage) Leadscrew(&spindle,&io,1000,100,1200,2,1200);
 GlobalState::getInstance()->setMotionMode(MM_ENABLED);
 for(auto* screw:{&automatic,zeroed}) {
  ASSERT_TRUE(screw->initializeOutputs()); screw->setFaultInhibit(false);
  GlobalState::getInstance()->setMotionMode(MM_ENABLED);
  for(uint32_t t=4;t<=1000;t+=4){setTestMicros(t);screw->update();}
  EXPECT_EQ(screw->getCurrentPosition(),0); EXPECT_EQ(io.readStepPin(),0);
  EXPECT_EQ(GlobalState::getInstance()->getThreadSyncState(),SS_UNSYNC);
 }
 zeroed->~Leadscrew();
}
TEST_F(Synchronization, EitherFirstStopEstablishesPhaseAndOtherStopPreservesIt) {
 for(auto stop:{LeadscrewStopPosition::LEFT,LeadscrewStopPosition::RIGHT}) {
  Spindle spindle; LeadscrewIOMock io; Leadscrew screw(&spindle,&io,1000,100,1200,2,1200);
  ASSERT_TRUE(screw.initializeOutputs()); screw.setFaultInhibit(false);
  screw.setTargetPitchMM(1); spindle.setCurrentPosition(100); spindle.consumePosition();
  screw.setStopPosition(stop);
  const auto other=stop==LeadscrewStopPosition::LEFT?LeadscrewStopPosition::RIGHT:LeadscrewStopPosition::LEFT;
  screw.setStopPosition(other,stop==LeadscrewStopPosition::LEFT?1000:-1000);
  expectPhase(screw,spindle,100,300);
  screw.unsetStopPosition(other);
  expectPhase(screw,spindle,-100,1100);
 }
}
TEST_F(Synchronization, ActiveAnchorTransfersBeforeOldCoordinateIsCleared) {
 Spindle spindle; LeadscrewIOMock io; Leadscrew screw(&spindle,&io,1000,100,1200,2,1200);
 ASSERT_TRUE(screw.initializeOutputs()); screw.setFaultInhibit(false);
 screw.setTargetPitchMM(1); spindle.setCurrentPosition(100); spindle.consumePosition();
 screw.setStopPosition(LeadscrewStopPosition::LEFT,0);
 screw.setStopPosition(LeadscrewStopPosition::RIGHT,100);
 screw.unsetStopPosition(LeadscrewStopPosition::LEFT);
 expectPhase(screw,spindle,100,300);
 expectPhase(screw,spindle,-100,1100);
 screw.unsetStopPosition(LeadscrewStopPosition::RIGHT);
 EXPECT_EQ(GlobalState::getInstance()->getThreadSyncState(),SS_UNSYNC);
}
TEST_F(Synchronization, ReplacingActiveAnchorUsesOldPhase) {
 Spindle spindle; LeadscrewIOMock io; Leadscrew screw(&spindle,&io,1000,100,1200,2,1200);
 ASSERT_TRUE(screw.initializeOutputs()); screw.setFaultInhibit(false);
 screw.setTargetPitchMM(1); spindle.setCurrentPosition(100); spindle.consumePosition();
 screw.setStopPosition(LeadscrewStopPosition::LEFT,0);
 screw.setStopPosition(LeadscrewStopPosition::LEFT,100);
 expectPhase(screw,spindle,100,300);
 screw.setStopPosition(LeadscrewStopPosition::LEFT,-100);
 expectPhase(screw,spindle,-100,1100);
}
TEST_F(Synchronization, RemoteFirstStopDoesNotManufactureRegistration) {
 Spindle spindle; LeadscrewIOMock io; Leadscrew screw(&spindle,&io,1000,100,1200,2,1200);
 ASSERT_TRUE(screw.initializeOutputs()); screw.setFaultInhibit(false);
 screw.setStopPosition(LeadscrewStopPosition::RIGHT,100);
 EXPECT_EQ(GlobalState::getInstance()->getThreadSyncState(),SS_UNSYNC);
}
TEST_F(Synchronization, ExtremeAnchorQuotientInhibitsInsteadOfWrapping) {
 Spindle spindle; LeadscrewIOMock io; Leadscrew screw(&spindle,&io,1000,100,1200,2,1200);
 ASSERT_TRUE(screw.initializeOutputs()); screw.setFaultInhibit(false);
 screw.setTargetPitchMM(1);
 screw.setCurrentPosition(INT32_MIN); screw.setStopPosition(LeadscrewStopPosition::LEFT);
 screw.setStopPosition(LeadscrewStopPosition::RIGHT,INT32_MAX);
 screw.unsetStopPosition(LeadscrewStopPosition::LEFT);
 GlobalState::getInstance()->setMotionMode(MM_ENABLED);
 screw.update();
 spindle.incrementCurrentPosition(600);
 for(uint32_t t=4;t<=1000;t+=4){setTestMicros(t);screw.update();}
 EXPECT_EQ(screw.getCurrentPosition(),INT32_MIN); EXPECT_EQ(io.readStepPin(),0);
 EXPECT_EQ(GlobalState::getInstance()->getThreadSyncState(),SS_UNSYNC);
}
TEST_F(Synchronization, InvalidPitchLatchesMotionInhibit) {
 for(float pitch:{0.0f,-1.0f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
  Spindle spindle; LeadscrewIOMock io; Leadscrew screw(&spindle,&io,1000,100,1200,2,1200);
  ASSERT_TRUE(screw.initializeOutputs()); screw.setFaultInhibit(false);
  screw.setTargetPitchMM(pitch); screw.setTargetPitchMM(1);
  GlobalState::getInstance()->setMotionMode(MM_ENABLED); spindle.incrementCurrentPosition(600);
  setTestMicros(1000); screw.update();
  EXPECT_EQ(screw.getCurrentPosition(),0); EXPECT_EQ(io.readStepPin(),0);
  EXPECT_EQ(GlobalState::getInstance()->getMotionMode(),MM_DISABLED);
 }
}
TEST_F(Synchronization, OutOfRangeJogCannotAdvancePhysicalPosition) {
 for(auto mode:{MM_JOG_LEFT,MM_JOG_RIGHT}) {
  Spindle spindle; LeadscrewIOMock io; Leadscrew screw(&spindle,&io,1000,100,1200,2,1200);
  ASSERT_TRUE(screw.initializeOutputs()); screw.setFaultInhibit(false);
  const int position=mode==MM_JOG_LEFT?INT32_MIN:INT32_MAX;
  screw.setCurrentPosition(position); GlobalState::getInstance()->setMotionMode(mode);
  screw.update();
  setTestMicros(elsMicros()+1000); screw.update();
  setTestMicros(elsMicros()+1000); screw.update();
  EXPECT_EQ(screw.getCurrentPosition(),position); EXPECT_EQ(io.readStepPin(),0);
 }
}

TEST_F(Synchronization, RightAnchorTransfersToLeftBeforeClearing) {
 Spindle spindle;LeadscrewIOMock io;Leadscrew screw(&spindle,&io,1000,100,1200,2,1200);
 ASSERT_TRUE(screw.initializeOutputs());screw.setFaultInhibit(false);
 screw.setTargetPitchMM(1);spindle.setCurrentPosition(100);spindle.consumePosition();
 screw.setStopPosition(LeadscrewStopPosition::RIGHT);
 screw.setStopPosition(LeadscrewStopPosition::LEFT,-100);
 screw.unsetStopPosition(LeadscrewStopPosition::RIGHT);
 expectPhase(screw,spindle,-100,1100);
 expectPhase(screw,spindle,100,300);
}

TEST_F(Synchronization, InvalidMachineRatioInputsCannotBeRearmed) {
 struct Machine {int motor;float pitch;int encoder;};
 for(const auto input:{Machine{0,2,1200},Machine{-1,2,1200},Machine{1200,0,1200},
     Machine{1200,-1,1200},Machine{1200,std::numeric_limits<float>::quiet_NaN(),1200},
     Machine{1200,std::numeric_limits<float>::infinity(),1200},Machine{1200,2,0},Machine{1200,2,-1}}) {
  Spindle spindle;LeadscrewIOMock io;Leadscrew screw(&spindle,&io,1000,100,input.motor,input.pitch,input.encoder);
  ASSERT_TRUE(screw.initializeOutputs());screw.setFaultInhibit(false);
  GlobalState::getInstance()->setMotionMode(MM_ENABLED);screw.update();
  spindle.incrementCurrentPosition(1200);
  for(int i=0;i<1000;++i){setTestMicros(elsMicros()+4);screw.update();}
  EXPECT_EQ(io.completions,0);EXPECT_EQ(screw.getCurrentPosition(),0);
  EXPECT_EQ(GlobalState::getInstance()->getMotionMode(),MM_DISABLED);
 }
}
