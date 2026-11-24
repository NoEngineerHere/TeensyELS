#include <gtest/gtest.h>
#include <encoder_detents.h>
#include <legacy_panel.h>
#include <globalstate.h>
#include "mocks/leadscrewio_mock.h"

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
  Controls controls(state,screw);LegacyPanel panel(controls);MatrixScanner scanner;
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
