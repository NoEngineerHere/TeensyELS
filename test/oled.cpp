#include <gtest/gtest.h>
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
 setTestMicros(0);GlobalState::getInstance()->setMotionMode(MM_DISABLED);
 Spindle spindle;LeadscrewIOMock io;Leadscrew screw(&spindle,&io,1000,100,1200,2,1200);
 ASSERT_TRUE(screw.initializeOutputs());
 OledSurface::Display display(&spindle,&screw);display.m_ssd1306.beginOK=false;
 EXPECT_FALSE(display.init());display.update();EXPECT_EQ(display.m_ssd1306.frames,0);
 screw.setFaultInhibit(false);GlobalState::getInstance()->setMotionMode(MM_ENABLED);screw.update();
 spindle.incrementCurrentPosition(1200);
 for(uint32_t t=4;t<1000;t+=4){setTestMicros(t);screw.update();}
 EXPECT_EQ(io.completions,0);
}
TEST(Oled, LockedAndFaultBannersAreFlushedToPanel) {
 setTestMicros(0);Spindle spindle;LeadscrewIOMock io;Leadscrew screw(&spindle,&io,1000,100,1200,2,1200);
 OledSurface::Display display(&spindle,&screw);ASSERT_TRUE(display.init());
 display.showBootStatus(false);display.showBootStatus(true);
 EXPECT_EQ(display.m_ssd1306.text,(std::vector<std::string>{"BOOT / LOCKED","BOOT / FAULT"}));
 EXPECT_EQ(display.m_ssd1306.frames,2);
}
