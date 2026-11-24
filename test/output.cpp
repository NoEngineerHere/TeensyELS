#include <gtest/gtest.h>
#include <leadscrew.h>
#include <globalstate.h>
#include "mocks/leadscrewio_mock.h"

struct ResetMotionState {
  explicit ResetMotionState(uint32_t time) {
    setTestMicros(time);
    auto* s = GlobalState::getInstance();
    s->setUnitMode(METRIC); s->setFeedMode(FM_FEED); s->setFeedSelect(-1);
    s->setButtonLock(LK_LOCKED); s->setMotionMode(MM_DISABLED); s->setThreadSyncState(SS_UNSYNC);
  }
};
struct MotionRig : ResetMotionState {
  Spindle spindle;
  LeadscrewIOMock io;
  Leadscrew screw;
  explicit MotionRig(uint32_t time=0) : ResetMotionState(time), screw(&spindle,&io,1000,100,1200,2,1200) {
    EXPECT_TRUE(screw.initializeOutputs()); screw.setFaultInhibit(false);
    screw.setTargetPitchMM(1); screw.update();
  }
  void tick(uint32_t dt=4) { setTestMicros(elsMicros()+dt); spindle.update(); screw.update(); }
  void enable(int delta) {
    GlobalState::getInstance()->setMotionMode(MM_ENABLED); screw.update();
    spindle.incrementCurrentPosition(delta);
  }
  bool high() {
    for(int i=0;i<1000 && !io.step;++i) tick();
    return io.step;
  }
};

TEST(Output, DisableLowAndRapidDisableEnableDiscardBacklog) {
 MotionRig r;
 r.spindle.incrementCurrentPosition(1200); r.tick();
 EXPECT_EQ(r.io.completions,0);
 r.enable(1200);
 GlobalState::getInstance()->setMotionMode(MM_DISABLED);
 GlobalState::getInstance()->setMotionMode(MM_ENABLED);
 r.tick();
 for(int i=0;i<1000;++i) r.tick();
 EXPECT_EQ(r.io.completions,0); EXPECT_TRUE(r.screw.isOutputIdle());
 EXPECT_EQ(r.screw.getPositionError(),0);
}
TEST(Output, DisableHighFinishesExactlyOnceInOriginalDirection) {
 for(int sign:{-1,1}) {
  MotionRig r(0xffffff90u); r.enable(sign*1200); ASSERT_TRUE(r.high());
  const uint32_t started=r.io.started;
  const int dir=r.io.dir;
  GlobalState::getInstance()->setMotionMode(MM_DISABLED);
  r.spindle.incrementCurrentPosition(-sign*1200);
  r.tick(ELS_STEP_HIGH_US-1);
  EXPECT_EQ(r.io.step,1); EXPECT_FALSE(r.screw.isOutputIdle()); EXPECT_EQ(r.io.dir,dir);
  r.tick(1);
  EXPECT_EQ(uint32_t(elsMicros()-started),ELS_STEP_HIGH_US);
  EXPECT_EQ(r.io.step,0); EXPECT_TRUE(r.screw.isOutputIdle());
  EXPECT_EQ(r.screw.getCurrentPosition(),sign); EXPECT_EQ(r.io.completions,1);
  for(int i=0;i<1000;++i)r.tick();
  EXPECT_EQ(r.io.completions,1); EXPECT_EQ(r.screw.getCurrentPosition(),sign);
  r.enable(-sign*20);
  const uint32_t rearmed=elsMicros();
  ASSERT_TRUE(r.high());
  EXPECT_GE(uint32_t(r.io.started-rearmed),100u);
  EXPECT_EQ(r.io.dir,sign>0?ELS_DIR_LEFT:ELS_DIR_RIGHT);
  for(int i=0;i<1000;++i)r.tick();
  EXPECT_EQ(r.screw.getCurrentPosition(),-sign*9);
 }
}
TEST(Output, InhibitClearAndFreshInitializationNeverEnable) {
 MotionRig r; r.enable(1200); ASSERT_TRUE(r.high());
 r.screw.setFaultInhibit(true); r.tick();
 r.screw.setFaultInhibit(false);
 EXPECT_EQ(GlobalState::getInstance()->getMotionMode(),MM_DISABLED);
 const int completed=r.io.completions;
 for(int i=0;i<1000;++i)r.tick();
 EXPECT_EQ(r.io.completions,completed);
 EXPECT_TRUE(r.screw.isOutputIdle());
 Leadscrew fresh(&r.spindle,&r.io,1000,100,1200,2,1200);
 GlobalState::getInstance()->setMotionMode(MM_ENABLED);
 r.spindle.incrementCurrentPosition(1000); fresh.update();
 EXPECT_EQ(fresh.getCurrentPosition(),0);
}
TEST(Output, InitializationSendAndCompletionFailuresStayLatched) {
 for(int failure:{0,1,2}) {
  ResetMotionState reset(0); Spindle spindle; LeadscrewIOMock io;
  io.beginOK=failure!=0; io.startOK=failure!=1;
  Leadscrew screw(&spindle,&io,1000,100,1200,2,1200);
  EXPECT_EQ(screw.initializeOutputs(),failure!=0);
  screw.setFaultInhibit(false); GlobalState::getInstance()->setMotionMode(MM_ENABLED); screw.update();
  spindle.incrementCurrentPosition(1200);
  for(uint32_t t=4;t<=1000;t+=4) {
    setTestMicros(t); if(failure==2 && io.step)io.completionOK=false; screw.update();
  }
  EXPECT_EQ(screw.getCurrentPosition(),0);
  screw.setFaultInhibit(false); GlobalState::getInstance()->setMotionMode(MM_ENABLED);
  spindle.incrementCurrentPosition(-1200);
  for(uint32_t t=1004;t<=2000;t+=4){setTestMicros(t);screw.update();}
  EXPECT_EQ(screw.getCurrentPosition(),0); EXPECT_EQ(io.completions,0);
  if(failure==2)EXPECT_FALSE(screw.isOutputIdle());
 }
}
TEST(Output, PulseTimingAndDirectionConstraintsHold) {
 MotionRig r; r.enable(1200);
 for(int i=0;i<5000;++i)r.tick();
 uint32_t lastRise=0,lastFall=0,lastDir=0;
 bool hadRise=false,hadFall=false;
 int step=0,dir=0;
 for(const auto& e:r.io.edges) {
  if(e.dir!=dir){ EXPECT_EQ(step,0); if(hadRise)EXPECT_GE(uint32_t(e.time-lastRise),ELS_DIR_HOLD_US); lastDir=e.time; }
  if(e.step && !step) {
   if(hadRise)EXPECT_GE(uint32_t(e.time-lastRise),8u);
   if(hadFall)EXPECT_GE(uint32_t(e.time-lastFall),ELS_STEP_LOW_US);
   EXPECT_GE(uint32_t(e.time-lastDir),ELS_DIR_SETUP_US);
   lastRise=e.time;hadRise=true;
  }
  if(!e.step && step){EXPECT_GE(uint32_t(e.time-lastRise),ELS_STEP_HIGH_US);lastFall=e.time;hadFall=true;}
  step=e.step;dir=e.dir;
 }
 EXPECT_EQ(r.screw.getCurrentPosition(),r.io.completions);
 EXPECT_GT(r.io.completions,1);
}

TEST(Output, ForegroundFatalInitializationInhibitCannotBeRearmed) {
 MotionRig r;
 r.screw.setFaultInhibit(true);
 r.screw.setFaultInhibit(false);
 r.enable(1200);
 for(int i=0;i<1000;++i)r.tick();
 EXPECT_EQ(r.screw.getCurrentPosition(),0);
 EXPECT_EQ(r.io.completions,0);
 EXPECT_TRUE(r.screw.isOutputIdle());
}

TEST(Output, DirectionSetupStartsAfterTheActualWriteNotCycleEntry) {
 ResetMotionState reset(0);
 struct DelayedDirIO : LeadscrewIOMock {
  uint32_t dirWrittenAt=0;
  void writeDirPin(uint8_t value) override {
   setTestMicros(elsMicros()+30);
   LeadscrewIOMock::writeDirPin(value);
   dirWrittenAt=elsMicros();
  }
 } io;
 Spindle spindle;Leadscrew screw(&spindle,&io,1000,100,1200,2,1200);
 ASSERT_TRUE(screw.initializeOutputs());screw.setFaultInhibit(false);
 GlobalState::getInstance()->setMotionMode(MM_ENABLED);screw.update();
 spindle.incrementCurrentPosition(1200);setTestMicros(100);screw.update();
 EXPECT_EQ(io.step,0);
 setTestMicros(io.dirWrittenAt+ELS_DIR_SETUP_US-1);screw.update();EXPECT_EQ(io.step,0);
 setTestMicros(io.dirWrittenAt+ELS_DIR_SETUP_US);screw.update();EXPECT_EQ(io.step,1);
}

TEST(Output, PeriodicDisabledServiceAcrossTwoWrapsRearmsFromRest) {
 MotionRig r;
 r.enable(1200);ASSERT_TRUE(r.high());
 GlobalState::getInstance()->setMotionMode(MM_DISABLED);r.tick();
 const int stoppedAt=r.screw.getCurrentPosition();
 const int completions=r.io.completions;
 for(uint64_t elapsed=0;elapsed<(uint64_t(2)<<32);elapsed+=500000) {
  r.spindle.incrementCurrentPosition(-100);r.tick(500000);
 }
 EXPECT_EQ(r.io.completions,completions);EXPECT_EQ(r.screw.getCurrentPosition(),stoppedAt);
 r.enable(-20);const uint32_t armed=elsMicros();ASSERT_TRUE(r.high());
 EXPECT_EQ(uint32_t(r.io.started-armed),100u);
 for(int i=0;i<1000;++i)r.tick();
 EXPECT_EQ(r.io.completions,completions+10);
 EXPECT_EQ(r.screw.getCurrentPosition(),stoppedAt-10);
}

TEST(Output, FractionalFollowingErrorDoesNotStartAnExtraStep) {
 MotionRig r;r.enable(1);
 for(int i=0;i<1000;++i)r.tick();
 EXPECT_EQ(r.io.completions,0);
 r.spindle.incrementCurrentPosition(1);
 for(int i=0;i<1000;++i)r.tick();
 EXPECT_EQ(r.io.completions,1);EXPECT_EQ(r.screw.getCurrentPosition(),1);
 EXPECT_EQ(r.io.step,0);
}
