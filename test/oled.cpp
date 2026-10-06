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
 setTestMicros(0);GlobalState::getInstance()->setMotionMode(MM_DISABLED);OledSurface::Wire={};
 Spindle spindle;LeadscrewIOMock io;Leadscrew screw(&spindle,&io,1000,100,1200,2,1200);
 ASSERT_TRUE(screw.initializeOutputs());
 OledSurface::Display display(&spindle,&screw);display.m_ssd1306.beginOK=false;
 EXPECT_FALSE(display.init());display.update();
 screw.setFaultInhibit(false);GlobalState::getInstance()->setMotionMode(MM_ENABLED);screw.update();
 spindle.incrementCurrentPosition(1200);
 for(uint32_t t=4;t<1000;t+=4){setTestMicros(t);screw.update();}
 EXPECT_EQ(io.completions,0);
}
TEST(Oled, StartupDataTransferFailureDoesNotMistakeAllocationForCommunication) {
 for(uint8_t error:{2,4}) {
  setTestMicros(0);OledSurface::Wire={};OledSurface::Wire.failAt=2;OledSurface::Wire.failure=error;
  Spindle spindle;LeadscrewIOMock io;Leadscrew screw(&spindle,&io,1000,100,1200,2,1200);
  ASSERT_TRUE(screw.initializeOutputs());OledSurface::Display display(&spindle,&screw);
  EXPECT_FALSE(display.init());
  screw.setFaultInhibit(false);GlobalState::getInstance()->setMotionMode(MM_JOG_RIGHT);
  for(int i=0;i<1000;++i){setTestMicros(elsMicros()+4);screw.update();}
  EXPECT_EQ(io.completions,0);EXPECT_EQ(io.step,0);
 }
}
TEST(Oled, RuntimeDataFailureDrainsAcceptedPulseOnceAndNeverRearmsOrRetries) {
 setTestMicros(0);OledSurface::Wire={};Spindle spindle;LeadscrewIOMock io;
 Leadscrew screw(&spindle,&io,1000,100,1200,2,1200);ASSERT_TRUE(screw.initializeOutputs());
 OledSurface::Display display(&spindle,&screw);ASSERT_TRUE(display.init());screw.setFaultInhibit(false);
 GlobalState::getInstance()->setMotionMode(MM_JOG_RIGHT);
 for(int i=0;i<1000&&!io.step;++i){setTestMicros(elsMicros()+4);screw.update();}
 ASSERT_EQ(io.step,1);ASSERT_EQ(io.completions,0);
 OledSurface::Wire.failAt=OledSurface::Wire.transmissions+2;display.update();
 const int afterFailure=OledSurface::Wire.transmissions;
 screw.setFaultInhibit(false);GlobalState::getInstance()->setMotionMode(MM_JOG_RIGHT);
 for(int i=0;i<1000;++i){setTestMicros(elsMicros()+4);screw.update();display.update();}
 EXPECT_EQ(io.completions,1);EXPECT_EQ(screw.getCurrentPosition(),1);EXPECT_EQ(io.step,0);
 EXPECT_EQ(OledSurface::Wire.transmissions,afterFailure);
}
TEST(Oled, ShortWireWriteAlsoLatchesInhibit) {
 setTestMicros(0);OledSurface::Wire={};OledSurface::Wire.shortWrite=true;
 Spindle spindle;LeadscrewIOMock io;Leadscrew screw(&spindle,&io,1000,100,1200,2,1200);
 ASSERT_TRUE(screw.initializeOutputs());OledSurface::Display display(&spindle,&screw);EXPECT_FALSE(display.init());
 screw.setFaultInhibit(false);GlobalState::getInstance()->setMotionMode(MM_JOG_LEFT);
 for(int i=0;i<1000;++i){setTestMicros(elsMicros()+4);screw.update();}
 EXPECT_EQ(io.completions,0);EXPECT_EQ(io.step,0);
}
