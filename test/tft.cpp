#include <gtest/gtest.h>
#include <leadscrew.h>
#include <globalstate.h>
#include "mocks/leadscrewio_mock.h"
// Compile the production display implementation against a recording screen,
// without replacing its formatter, cache, class declaration or spindle math.
#undef ELS_DISPLAY
#define ELS_DISPLAY ST7789_240_135
#undef PIO_UNIT_TESTING
#include "../lib/display/ST7789_240_135display.cpp"
#define PIO_UNIT_TESTING 1
class RpmSpindle : public Spindle {
public:
 float rpm=12;
 float getEstimatedVelocityInRPM() override {return rpm;}
};
class TestDisplay : public Display {
public:
 using Display::Display;
 using Display::drawSpindleRpm;
};
TEST(Tft, ValidInvalidTransitionsRedrawOnlyChangedCompleteStrings) {
 setTestMicros(0);RpmSpindle spindle;LeadscrewIOMock io;
 Leadscrew screw(&spindle,&io,1000,100,1200,2,1200);
 TestDisplay display(&spindle,&screw);ASSERT_TRUE(display.init());
 display.drawSpindleRpm();display.drawSpindleRpm();
 spindle.rpm=1000000;display.drawSpindleRpm();display.drawSpindleRpm();
 spindle.rpm=12;display.drawSpindleRpm();
 ASSERT_EQ(display.tft.text,(std::vector<std::string>{"  12RPM"," ---RPM","  12RPM"}));
 ASSERT_EQ(display.tft.cleared.size(),3u);
 for(const auto& rect:display.tft.cleared){EXPECT_EQ(rect.x,100);EXPECT_EQ(rect.w,126);EXPECT_LE(rect.x+rect.w,240);}
}

TEST(Tft, BootBannerUsesLockedAndFaultStates) {
 setTestMicros(0);RpmSpindle spindle;LeadscrewIOMock io;
 Leadscrew screw(&spindle,&io,1000,100,1200,2,1200);
 TestDisplay display(&spindle,&screw);ASSERT_TRUE(display.init());
 display.showBootStatus(false);display.showBootStatus(true);
 EXPECT_EQ(display.tft.text,(std::vector<std::string>{"BOOT / LOCKED","BOOT / FAULT"}));
 EXPECT_EQ(io.completions,0);
}
