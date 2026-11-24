#include <gtest/gtest.h>
#include <array>
#include <leadscrew.h>
#include <globalstate.h>
#include <boot_diagnostics.h>
#include <rpm_format.h>
#include <string>
#include <vector>
#include "mocks/leadscrewio_mock.h"
namespace OledSurface {
#undef PIO_UNIT_TESTING
#include "../lib/display/SSD1306_128_64display.cpp"
#define PIO_UNIT_TESTING 1
}
TEST(Oled, AllocationFailureReturnsAndLatchesMotionInhibit) {
 GlobalState state;setTestMicros(0);state.setMotionMode(MM_DISABLED);OledSurface::Wire={};
 Spindle spindle;LeadscrewIOMock io;Leadscrew screw(state, &spindle,&io,testPulseTiming,1000,100,1200,2,1200);
 ASSERT_TRUE(screw.initializeOutputs());
 OledSurface::Display display(state, &spindle,&screw);display.m_ssd1306.beginOK=false;
 EXPECT_FALSE(display.init());display.update();
 screw.setFaultInhibit(false);state.setMotionMode(MM_ENABLED);screw.update();
 spindle.incrementCurrentPosition(1200);
 for(uint32_t t=4;t<1000;t+=4){setTestMicros(t);screw.update();}
 EXPECT_EQ(io.completions,0);
}
TEST(Oled, StartupDataTransferFailureDoesNotMistakeAllocationForCommunication) {
 for(uint8_t error:{2,4}) {
  GlobalState state;setTestMicros(0);OledSurface::Wire={};OledSurface::Wire.failAt=35;OledSurface::Wire.failure=error;
  Spindle spindle;LeadscrewIOMock io;Leadscrew screw(state, &spindle,&io,testPulseTiming,1000,100,1200,2,1200);
  ASSERT_TRUE(screw.initializeOutputs());OledSurface::Display display(state, &spindle,&screw);
  ASSERT_TRUE(display.init());display.showBootStatus(false);
  for (uint32_t ms=5;ms<=175;ms+=5) EXPECT_FALSE(display.service(ms));
  EXPECT_TRUE(display.failed());EXPECT_EQ(OledSurface::Wire.transmissions,35);
  screw.setFaultInhibit(false);state.setMotionMode(MM_JOG_RIGHT);
  for(int i=0;i<1000;++i){setTestMicros(elsMicros()+4);screw.update();}
  EXPECT_EQ(io.completions,0);EXPECT_EQ(io.step,0);
 }
}
TEST(Oled, RuntimeDataFailureDrainsAcceptedPulseOnceAndNeverRearmsOrRetries) {
 GlobalState state;setTestMicros(0);OledSurface::Wire={};Spindle spindle;LeadscrewIOMock io;
 Leadscrew screw(state, &spindle,&io,testPulseTiming,1000,100,1200,2,1200);ASSERT_TRUE(screw.initializeOutputs());
 OledSurface::Display display(state, &spindle,&screw);ASSERT_TRUE(display.init());screw.setFaultInhibit(false);
 state.setMotionMode(MM_JOG_RIGHT);
 for(int i=0;i<1000&&!io.step;++i){setTestMicros(elsMicros()+4);screw.update();}
 ASSERT_EQ(io.step,1);ASSERT_EQ(io.completions,0);
 OledSurface::Wire.failAt=OledSurface::Wire.transmissions+2;display.update();
 display.service(5);display.service(10);
 const int afterFailure=OledSurface::Wire.transmissions;
 screw.setFaultInhibit(false);state.setMotionMode(MM_JOG_RIGHT);
 for(int i=0;i<1000;++i){setTestMicros(elsMicros()+4);screw.update();display.update();}
 EXPECT_EQ(io.completions,1);EXPECT_EQ(screw.getCurrentPosition(),1);EXPECT_EQ(io.step,0);
 EXPECT_EQ(OledSurface::Wire.transmissions,afterFailure);
}
TEST(Oled, ShortWireWriteAlsoLatchesInhibit) {
 GlobalState state;setTestMicros(0);OledSurface::Wire={};OledSurface::Wire.shortWrite=true;
 Spindle spindle;LeadscrewIOMock io;Leadscrew screw(state, &spindle,&io,testPulseTiming,1000,100,1200,2,1200);
 ASSERT_TRUE(screw.initializeOutputs());OledSurface::Display display(state, &spindle,&screw);ASSERT_TRUE(display.init());
 display.showBootStatus(false);EXPECT_FALSE(display.service(5));EXPECT_TRUE(display.failed());
 screw.setFaultInhibit(false);state.setMotionMode(MM_JOG_LEFT);
 for(int i=0;i<1000;++i){setTestMicros(elsMicros()+4);screw.update();}
 EXPECT_EQ(io.completions,0);EXPECT_EQ(io.step,0);
}

TEST(Oled, FullFrameReachesWireWithinOneStatusInterval) {
 GlobalState state;Spindle spindle;LeadscrewIOMock io;
 Leadscrew screw(state, &spindle,&io,testPulseTiming,1000,100,1200,2,1200);
 OledSurface::Wire={};OledSurface::Display display(state,&spindle,&screw);
 ASSERT_TRUE(display.init());display.showBootStatus(false);
 for (uint32_t ms=5;ms<175;ms+=5) EXPECT_FALSE(display.service(ms));
 EXPECT_TRUE(display.service(175));EXPECT_FALSE(display.failed());
 EXPECT_EQ(OledSurface::Wire.transmissions,35);EXPECT_EQ(OledSurface::Wire.bytes,6+1024);
}
