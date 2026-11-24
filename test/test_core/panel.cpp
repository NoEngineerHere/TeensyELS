#include <gtest/gtest.h>
#include <encoder_detents.h>
#include <legacy_panel.h>
#include <mk2_panel.h>
#include <globalstate.h>
#include "../mocks/leadscrewio_mock.h"

static MatrixEvent scan(MatrixScanner& scanner, uint16_t mask, uint32_t now) {
  scanner.sample(mask, now);
  return scanner.takeEvent();
}

TEST(MatrixScanner, DebouncesPressAndEmitsEitherClickOrOneHoldAcrossWrap) {
  MatrixScanner scanner;
  constexpr uint16_t key = matrixBit(1,0);
  constexpr uint32_t start = UINT32_MAX - 12;
  EXPECT_EQ(scan(scanner,key,start).gesture,MatrixGesture::None);
  EXPECT_EQ(scan(scanner,0,start+5u).gesture,MatrixGesture::None);
  EXPECT_EQ(scan(scanner,key,start+6u).gesture,MatrixGesture::None);
  EXPECT_EQ(scan(scanner,key,start+15u).gesture,MatrixGesture::None);
  const auto pressed = scan(scanner,key,start+16u);
  EXPECT_EQ(pressed.gesture,MatrixGesture::Press);EXPECT_EQ(pressed.key,3);
  EXPECT_EQ(scan(scanner,key,start+1015u).gesture,MatrixGesture::None);
  EXPECT_EQ(scan(scanner,key,start+1016u).gesture,MatrixGesture::Hold);
  EXPECT_EQ(scan(scanner,key,start+2050u).gesture,MatrixGesture::None);
  scan(scanner,0,start+2055u);
  EXPECT_EQ(scan(scanner,0,start+2065u).gesture,MatrixGesture::None);
  scan(scanner,key,start+2070u);scan(scanner,key,start+2080u);
  scan(scanner,0,start+2110u);
  const auto click = scan(scanner,0,start+2120u);
  EXPECT_EQ(click.gesture,MatrixGesture::Click);EXPECT_EQ(click.key,3);
}

TEST(MatrixScanner, HaltIsIndependentOfChordAndSuppressesTrailingEnable) {
  constexpr uint16_t halt = matrixBit(2,0), enable = matrixBit(2,2), other = matrixBit(0,0);
  MatrixScanner scanner(halt);
  scan(scanner,enable,0);
  EXPECT_EQ(scan(scanner,enable,10).gesture,MatrixGesture::Press);
  scan(scanner,enable|halt,15);
  scan(scanner,enable|halt|other,20);
  const auto stop = scan(scanner,enable|halt,25);
  EXPECT_EQ(stop.gesture,MatrixGesture::Press);EXPECT_EQ(stop.key,6);
  EXPECT_EQ(scan(scanner,halt|enable,1035).gesture,MatrixGesture::None);
  for (const auto ms : {1040u,1050u,2100u})
    EXPECT_EQ(scan(scanner,enable,ms).gesture,MatrixGesture::None);
  scan(scanner,0,2105);
  EXPECT_EQ(scan(scanner,0,2115).gesture,MatrixGesture::None);
  EXPECT_FALSE(scanner.blocked());
  scan(scanner,enable,2120);scan(scanner,enable,2130);scan(scanner,0,2135);
  const auto click = scan(scanner,0,2145);
  EXPECT_EQ(click.gesture,MatrixGesture::Click);EXPECT_EQ(click.key,8);
}

TEST(MatrixScanner, LegacyChordDoesNotBecomeAJogOnRelease) {
  MatrixScanner scanner;
  constexpr auto left = matrixBit(2,1), right = matrixBit(2,2);
  scan(scanner,left,0);scan(scanner,left,10);scan(scanner,left|right,20);
  scan(scanner,right,30);
  EXPECT_EQ(scan(scanner,right,1040).gesture,MatrixGesture::None);
  scan(scanner,0,1050);
  EXPECT_EQ(scan(scanner,0,1060).gesture,MatrixGesture::None);
  EXPECT_FALSE(scanner.blocked());
}

TEST(Panel, DetentsRetainPartialCountsAndCancelOnReversal) {
  EncoderDetents detents(4);
  EXPECT_EQ(detents.consume(3),0);
  EXPECT_EQ(detents.consume(-5),0);
  EXPECT_EQ(detents.consume(-2),-1);
  EXPECT_EQ(detents.consume(7),1);
  EXPECT_EQ(detents.consume(1),1);
}

TEST(Panel, LegacyPhysicalPositionsPreserveExistingClickAndHoldBindings) {
  GlobalState state;Spindle spindle;LeadscrewIOMock io;
  Leadscrew screw(state,&spindle,&io,testPulseTiming,1000,100,1200,2,1200);
  Controls controls(state,screw);LegacyPanel panel(controls,false);MatrixScanner scanner;
  uint32_t time=0;
  const auto sample = [&](uint16_t mask,uint32_t ms){time+=ms;panel.key(scan(scanner,mask,time));};
  const auto press = [&](uint16_t mask,bool hold=false){
    sample(mask,5);sample(mask,10);
    if (hold) sample(mask,1000);
    sample(0,5);sample(0,10);
  };
  press(matrixBit(2,0));EXPECT_EQ(state.getButtonLock(),LK_UNLOCKED);
  press(matrixBit(0,2),true);EXPECT_EQ(state.getUnitMode(),IMPERIAL);EXPECT_EQ(state.getFeedMode(),FM_FEED);
  press(matrixBit(0,2));EXPECT_EQ(state.getFeedMode(),FM_THREAD);
  press(matrixBit(2,1),true);EXPECT_EQ(screw.getStopPositionState(LeadscrewStopPosition::LEFT),LeadscrewStopState::SET);
  press(matrixBit(2,1));EXPECT_EQ(state.getMotionMode(),MM_JOG_LEFT);
  press(matrixBit(2,1));EXPECT_EQ(state.getMotionMode(),MM_DISABLED);
  press(matrixBit(1,2));EXPECT_EQ(state.getMotionMode(),MM_ENABLED);
}

struct Mk2Rig {
  GlobalState state;
  Spindle spindle;
  LeadscrewIOMock io;
  Leadscrew screw{state,&spindle,&io,testPulseTiming,1000,100,1200,2,1200};
  Controls controls{state,screw};
  Mk2Panel panel;
  MatrixScanner scanner{Mk2Panel::haltMask};
  uint32_t now=0;
  explicit Mk2Rig(bool updates=false) : panel(controls,updates) {}
  void sample(uint16_t mask, uint32_t delay) {now+=delay;panel.key(scan(scanner,mask,now));}
  void press(unsigned row,unsigned col,bool hold=false) {
    const auto mask=matrixBit(row,col);
    sample(mask,5);sample(mask,10);
    if(hold) sample(mask,1000);
    sample(0,5);sample(0,10);
  }
};

TEST(Mk2, MenuUnlocksAtBootAndUpdatesOnlyOnExplicitOk) {
  Mk2Rig rig(true);
  rig.press(2,1);EXPECT_EQ(rig.panel.selection().view,Mk2View::Menu);
  EXPECT_EQ(rig.state.getButtonLock(),LK_LOCKED);
  rig.press(1,1);EXPECT_EQ(rig.state.getButtonLock(),LK_UNLOCKED);
  rig.press(1,0);EXPECT_EQ(rig.panel.selection().menu,unsigned(Mk2Menu::Update));
  EXPECT_FALSE(rig.state.hasOTA());
  rig.press(1,1,true);EXPECT_FALSE(rig.state.hasOTA());
  rig.press(1,1);EXPECT_TRUE(rig.state.hasOTA());

  Mk2Rig wired;
  wired.press(2,1);wired.panel.turn(-1);
  EXPECT_EQ(wired.panel.selection().menu,unsigned(Mk2Menu::ResetDisplay));
  wired.panel.turn(1);EXPECT_EQ(wired.panel.selection().menu,unsigned(Mk2Menu::Lock));
}

TEST(Mk2, ViewsRouteArrowsAndRotaryWithoutJoggingOnMenuNavigation) {
  Mk2Rig rig;
  rig.press(2,1);rig.press(1,1);rig.press(2,1);
  rig.press(0,2);EXPECT_EQ(rig.panel.selection().view,Mk2View::Stops);
  rig.panel.turn(1);EXPECT_EQ(rig.panel.selection().stop,LeadscrewStopPosition::RIGHT);
  rig.press(1,1);EXPECT_EQ(rig.screw.getStopPositionState(LeadscrewStopPosition::RIGHT),LeadscrewStopState::SET);
  rig.press(1,2);EXPECT_EQ(rig.state.getMotionMode(),MM_DISABLED);
  rig.press(2,1);rig.press(1,2);rig.press(1,2,true);
  EXPECT_EQ(rig.state.getMotionMode(),MM_DISABLED);EXPECT_EQ(rig.panel.selection().menu,unsigned(Mk2Menu::Units));
  rig.press(1,1);EXPECT_EQ(rig.state.getUnitMode(),IMPERIAL);
  rig.press(0,1);EXPECT_EQ(rig.panel.selection().view,Mk2View::Rate);
  const int pitch=rig.state.getFeedSelect();rig.panel.turn(1);EXPECT_NE(rig.state.getFeedSelect(),pitch);
  rig.press(1,2);EXPECT_EQ(rig.state.getMotionMode(),MM_JOG_RIGHT);
}

TEST(Mk2, HaltDisablesEvenLockedAndSuppressesAnOverlappedEnableRelease) {
  for (auto mode : {MM_ENABLED,MM_JOG_LEFT,MM_JOG_RIGHT}) {
    Mk2Rig rig;
    rig.state.setMotionMode(mode);
    constexpr auto halt=matrixBit(2,0), enable=matrixBit(2,2);
    rig.sample(enable,5);rig.sample(enable,10);
    rig.sample(halt|enable,5);rig.sample(halt|enable,10);
    EXPECT_EQ(rig.state.getMotionMode(),MM_DISABLED);
    rig.state.setButtonLock(LK_UNLOCKED);
    rig.sample(enable,5);rig.sample(enable,10);rig.sample(0,5);rig.sample(0,10);
    EXPECT_EQ(rig.state.getMotionMode(),MM_DISABLED);
    rig.press(2,2);EXPECT_EQ(rig.state.getMotionMode(),MM_ENABLED);
  }
}
