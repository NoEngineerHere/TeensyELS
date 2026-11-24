#include <gtest/gtest.h>
#include <display_status.h>
#include <display_bus.h>
#include <Adafruit_ST7789.h>
#include "../mocks/leadscrewio_mock.h"

#undef ELS_SCREEN
#define ELS_SCREEN ELS_SCREEN_ST7789_240X135
#undef ELS_HARDWARE_CONFIG
#define ELS_HARDWARE_CONFIG "native_tft.h"
namespace TftSurface {
#define ELS_TEST_TEENSY_BUS
#include "../../lib/board/display_bus_teensy.cpp"
#undef ELS_TEST_TEENSY_BUS
#define ELS_TEST_TFT
#include "../../lib/display/ST7789_240_135display.cpp"
#undef ELS_TEST_TFT
}
class RpmSpindle : public Spindle {
public:
 float rpm=12;
 float getEstimatedVelocityInRPM() override {return rpm;}
};
class Tft : public testing::Test {
protected:
 GlobalState state;
 RpmSpindle spindle;
 LeadscrewIOMock io;
 Leadscrew screw{state,&spindle,&io,testPulseTiming,1000,100,1200,2,1200};
 Controls controls{state,screw};
 Mk2Panel panel{controls,true};
};

TEST_F(Tft, ConfiguredSpiAndControlPinsReachThePortableDriver) {
 TftSurface::Display display(state,&spindle,&screw);
 const auto begins=SPI.begins;
 ASSERT_TRUE(TftSurface::initializeDisplayBus());ASSERT_TRUE(display.init());
 EXPECT_EQ(SPI.mosi,11);EXPECT_EQ(SPI.miso,12);EXPECT_EQ(SPI.sclk,13);
 EXPECT_GT(SPI.begins,begins);EXPECT_EQ(display.tft.spi,&SPI);
 EXPECT_EQ(display.tft.cs,20);EXPECT_EQ(display.tft.dc,21);EXPECT_EQ(display.tft.reset,22);
 EXPECT_EQ(display.tft.width,240);EXPECT_EQ(display.tft.height,135);EXPECT_TRUE(display.tft.inverted);
 EXPECT_EQ(display.tft.frequency,40000000u);
}
TEST_F(Tft, ValidInvalidTransitionsRedrawOnlyChangedCompleteRpmStrings) {
 TftSurface::Display display(state,&spindle,&screw);
 ASSERT_TRUE(TftSurface::initializeDisplayBus());ASSERT_TRUE(display.init());
 display.update();display.update();
 spindle.rpm=1000000;display.update();display.update();
 spindle.rpm=12;display.update();
 std::vector<std::string> rpm;
 for(const auto& entry:display.tft.printed) if(entry.x==100 && entry.y==0) rpm.push_back(entry.value);
 EXPECT_EQ(rpm,(std::vector<std::string>{"  12RPM"," ---RPM","  12RPM"}));
 int cleared=0;
 for(const auto& rect:display.tft.cleared) if(rect.x==100 && rect.y==0) {
  ++cleared;EXPECT_EQ(rect.w,126);EXPECT_LE(rect.x+rect.w,240);
 }
 EXPECT_EQ(cleared,3);
}
TEST_F(Tft, BootBannerAndMk2MenuUseTheSameTransport) {
 TftSurface::Display display(state,&spindle,&screw,&panel);
 ASSERT_TRUE(TftSurface::initializeDisplayBus());ASSERT_TRUE(display.init());
 display.showBootStatus(false);display.showBootStatus(true);
 EXPECT_EQ(display.tft.text,(std::vector<std::string>{"BOOT / LOCKED","BOOT / FAULT"}));
 display.tft.text.clear();display.tft.printed.clear();
 panel.key({MatrixGesture::Click,7});panel.turn(1);display.update();
 ASSERT_EQ(display.tft.text.size(),7u);
 EXPECT_EQ(display.tft.text[0].find("Menu"),0u);EXPECT_EQ(display.tft.text[5].find("> Units"),0u);
 for(const auto& row:display.tft.printed) {
  EXPECT_LE(row.x+int(row.value.size())*6*row.size,240);
  EXPECT_LE(row.y+8*row.size,135);
 }
 display.tft.capture("tft-menu");
}

TEST_F(Tft, CarriageReadoutPreservesSignAndSwitchesDatum) {
 TftSurface::Display display(state,&spindle,&screw);
 ASSERT_TRUE(TftSurface::initializeDisplayBus());ASSERT_TRUE(display.init());
 ASSERT_TRUE(screw.initializeOutputs());
 using Stop=LeadscrewStopPosition;
 screw.setCurrentPosition(0);
 screw.setStopPosition(Stop::RIGHT,1200);
 display.update();
 auto readout=[&] {
  for(auto it=display.tft.printed.rbegin();it!=display.tft.printed.rend();++it)
   if(it->y==56) return it->value.substr(0,it->value.find_last_not_of(' ')+1);
  return std::string{};
 };
 EXPECT_EQ(readout(),"Right -2.000mm");
 screw.setStopPosition(Stop::LEFT,-600);
 display.update();
 EXPECT_EQ(readout(),"Left +1.000mm");
 state.setUnitMode(IMPERIAL);
 display.update();
 EXPECT_EQ(readout(),"Left +0.039in");
}
