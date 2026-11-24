#include <gtest/gtest.h>
#include <display_status.h>
#include <display_bus.h>
#include <Adafruit_SSD1306.h>
#include <algorithm>
#include "../mocks/leadscrewio_mock.h"

#undef ELS_NATIVE_PROCESSOR
#define ELS_NATIVE_PROCESSOR Processor::Esp32
#undef ELS_HARDWARE_CONFIG
#define ELS_HARDWARE_CONFIG "native_oled.h"
namespace OledSurface {
#define ELS_TEST_ESP_BUS
#include "../../lib/board/display_bus_esp32.cpp"
#undef ELS_TEST_ESP_BUS
#define ELS_TEST_OLED
#include "../../lib/display/SSD1306_128_64display.cpp"
#undef ELS_TEST_OLED
}

class Oled : public testing::Test {
protected:
 GlobalState state;
 Spindle spindle;
 LeadscrewIOMock io;
 Leadscrew screw{state,&spindle,&io,testPulseTiming,1000,100,1200,2,1200};
 Controls controls{state,screw};
 Mk2Panel panel{controls,true};
 void SetUp() override { Wire={};Wire1={}; }
};

TEST_F(Oled, AllocationFailureLatchesAndDoesNotTryToTransfer) {
 OledSurface::Display display(state,&spindle,&screw);
 ASSERT_TRUE(OledSurface::initializeDisplayBus());
 display.m_ssd1306.beginOK=false;
 EXPECT_FALSE(display.init());display.showBootStatus(false);display.update();
 EXPECT_FALSE(display.service(5));EXPECT_TRUE(display.failed());
 EXPECT_EQ(Wire1.transmissions,0);
}

TEST_F(Oled, FullBootFrameUsesOnlyConfiguredWireAndAddress) {
 OledSurface::Display display(state,&spindle,&screw);
 EXPECT_EQ(Wire1.begins,0);
 ASSERT_TRUE(OledSurface::initializeDisplayBus());ASSERT_TRUE(display.init());
 EXPECT_EQ(Wire1.sda,19);EXPECT_EQ(Wire1.scl,18);EXPECT_EQ(Wire1.clock,100000u);
 EXPECT_EQ(display.m_ssd1306.wire,&Wire1);EXPECT_FALSE(display.m_ssd1306.periphBegin);
 EXPECT_EQ(display.m_ssd1306.initAddress,0x3d);
 display.showBootStatus(false);display.update();
 EXPECT_EQ(display.m_ssd1306.text,(std::vector<std::string>{"BOOT / LOCKED"}));
 for(uint32_t ms=5;ms<175;ms+=5) EXPECT_FALSE(display.service(ms));
 EXPECT_TRUE(display.service(175));EXPECT_FALSE(display.failed());
 EXPECT_EQ(Wire1.transmissions,35);EXPECT_EQ(Wire1.bytes,6+1024);
 EXPECT_EQ(Wire1.addresses,std::vector<uint8_t>(35,0x3d));
 EXPECT_EQ(Wire.begins,0);EXPECT_EQ(Wire.transmissions,0);
}

TEST_F(Oled, BusBeginErrorsAreReported) {
 Wire1.beginOK=false;
 EXPECT_FALSE(OledSurface::initializeDisplayBus());
 EXPECT_EQ(Wire.begins,0);
}

TEST_F(Oled, FailedFinalChunkAndShortWriteLatchWithoutRetry) {
 for(bool shortWrite:{false,true}) {
  Wire1={};Wire1.failAt=shortWrite?-1:35;Wire1.shortWrite=shortWrite;
  OledSurface::Display display(state,&spindle,&screw);
  ASSERT_TRUE(OledSurface::initializeDisplayBus());ASSERT_TRUE(display.init());
  display.showBootStatus(false);
  for(uint32_t ms=5;ms<=175;ms+=5) EXPECT_FALSE(display.service(ms));
  EXPECT_TRUE(display.failed());EXPECT_EQ(Wire1.transmissions,shortWrite?1:35);
  EXPECT_EQ(Wire1.clock,100000u);
  display.update();EXPECT_FALSE(display.service(180));EXPECT_FALSE(display.init());
 }
}

TEST_F(Oled, Mk2MenuFitsTheTextGrid) {
 OledSurface::Display display(state,&spindle,&screw,&panel);
 ASSERT_TRUE(OledSurface::initializeDisplayBus());ASSERT_TRUE(display.init());
 panel.key({MatrixGesture::Click,7});panel.turn(1);display.update();
 const auto& text=display.m_ssd1306.text;
 ASSERT_EQ(text.size(),7u);
 EXPECT_EQ(text[0],"Menu");EXPECT_EQ(text[4],"  Unlock");
 EXPECT_EQ(text[5],"> Units");EXPECT_EQ(text[6],"  Sync");
 for(const auto& row:display.m_ssd1306.printed) {
  EXPECT_LE(row.x+int(row.value.size())*6*row.size,128);
  EXPECT_LE(row.y+8*row.size,64);
 }
 display.m_ssd1306.capture("oled-menu");
}

TEST_F(Oled, MaintenanceProgressReplacesCredentialsAndFailureClearsThem) {
 OledSurface::Display display(state,&spindle,&screw,&panel);
 ASSERT_TRUE(OledSurface::initializeDisplayBus());ASSERT_TRUE(display.init());
 ASSERT_TRUE(screw.initializeOutputs());screw.setFaultInhibit(false);
 state.setButtonLock(LK_UNLOCKED);ASSERT_TRUE(screw.requestMaintenance());
 UpdateStatus waiting{UpdatePhase::Waiting,"ELS-12AB34","00112233445566778899aabbccddeeff","192.168.4.1"};
 state.publishUpdate(waiting);display.update();
 EXPECT_EQ(display.m_ssd1306.text[0],"Join ELS-12AB34");
 EXPECT_EQ(display.m_ssd1306.text[2],"0011223344556677");
 EXPECT_EQ(display.m_ssd1306.text[3],"8899aabbccddeeff");
 for(uint32_t ms=5;ms<=175;ms+=5) display.service(ms);
 UpdateStatus receiving{UpdatePhase::Receiving};receiving.received=12345;receiving.total=23456;
 state.publishUpdate(receiving);display.m_ssd1306.text.clear();display.update();
 EXPECT_EQ(display.m_ssd1306.text[1],"12345/23456");
 for(uint32_t ms=180;ms<=350;ms+=5) display.service(ms);
 UpdateStatus failure{UpdatePhase::Failed};failure.detail="Wrong hardware";
 state.publishUpdate(failure);display.m_ssd1306.text.clear();display.update();
 EXPECT_EQ(display.m_ssd1306.text[1],"Wrong hardware");
 EXPECT_EQ(display.m_ssd1306.text[2],"Reboot locally");
 EXPECT_TRUE(display.m_ssd1306.text[3].empty());
 screw.setFaultInhibit(false);state.setMotionMode(MM_ENABLED);
 EXPECT_EQ(state.getMotionMode(),MM_DISABLED);
}

TEST_F(Oled, CarriageReadoutLabelsOriginAndZeroAtASetStopWithoutMovingIt) {
 OledSurface::Display display(state,&spindle,&screw);
 ASSERT_TRUE(OledSurface::initializeDisplayBus());ASSERT_TRUE(display.init());
 ASSERT_TRUE(screw.initializeOutputs());
 screw.setCurrentPosition(600);
 display.update();
 ASSERT_FALSE(display.m_ssd1306.printed.empty());
 EXPECT_EQ(display.m_ssd1306.printed.back().value,"Origin +1.000mm");
 EXPECT_EQ(display.m_ssd1306.printed.back().y,56);
 for(uint32_t ms=5;ms<=175;ms+=5) display.service(ms);
 const auto stop=LeadscrewStopPosition::LEFT;
 screw.setStopPosition(stop);
 state.setThreadSyncState(SS_UNSYNC);
 display.update();
 EXPECT_EQ(display.m_ssd1306.printed.back().value,"Left +0.000mm");
 EXPECT_EQ(screw.getCurrentPosition(),600);
 EXPECT_EQ(screw.getStopPosition(stop),600);
 EXPECT_EQ(state.getThreadSyncState(),SS_UNSYNC);
 EXPECT_EQ(state.getMotionMode(),MM_DISABLED);
 EXPECT_EQ(io.completions,0);
}

TEST_F(Oled, ReverseThreadingIsLabeledOnBothPanelLayouts) {
 state.setFeedMode(FM_THREAD_REVERSE);
 ASSERT_TRUE(OledSurface::initializeDisplayBus());
 OledSurface::Display legacy(state,&spindle,&screw);
 ASSERT_TRUE(legacy.init()); legacy.update();
 const auto& text=legacy.m_ssd1306.text;
 EXPECT_NE(std::find(text.begin(),text.end(),"Rev"),text.end());
 OledSurface::Display mk2(state,&spindle,&screw,&panel);
 ASSERT_TRUE(mk2.init()); mk2.update();
 ASSERT_GE(mk2.m_ssd1306.text.size(),2u);
 EXPECT_EQ(mk2.m_ssd1306.text[1].find("Reverse "),0u);
}
