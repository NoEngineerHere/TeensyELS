#include <gtest/gtest.h>
#include <display.h>
#include "../mocks/leadscrewio_mock.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>

static void capture(const Adafruit_ST7789& panel, const char* name) {
  const char* dir=std::getenv("ELS_RENDER_DIR");
  if(!dir) return;
  std::ofstream out(std::string(dir)+"/"+name+".ppm",std::ios::binary);
  out << "P6\n" << panel.width << ' ' << panel.height << "\n255\n";
  for(const auto pixel: panel.pixels) {
    out.put(char(((pixel>>11)&31)*255/31));
    out.put(char(((pixel>>5)&63)*255/63));
    out.put(char((pixel&31)*255/31));
  }
}

TEST(Lvgl, FlushesBootMk2MenuAndFaultWithBoundedRgb565Transfers) {
  GlobalState state;Spindle spindle;LeadscrewIOMock io;
  Leadscrew screw(state,&spindle,&io,testPulseTiming,1000,100,1200,2,1200);
  Controls controls(state,screw);Mk2Panel panel(controls,true);
  Display display(state,&spindle,&screw,&panel);
  ASSERT_TRUE(initializeDisplayBus());ASSERT_TRUE(display.init());
  auto* screen=lv_display_get_screen_active(lv_display_get_default());
  const auto label = [&](int i){return lv_label_get_text(lv_obj_get_child(screen,i));};
  uint32_t time=0;
  const auto flush = [&](){for(unsigned i=0;i<8;++i){time+=5;display.service(time);}};

  display.showBootStatus(false);flush();
  EXPECT_STREQ(label(0),"BOOT / LOCKED");
  const auto boot=display.tft.pixels;
  ASSERT_FALSE(boot.empty());
  EXPECT_TRUE(std::any_of(boot.begin(),boot.end(),[&](auto px){return px!=boot.front();}));
  capture(display.tft,"lvgl-boot");

  panel.key({MatrixGesture::Click,7});
  panel.key({MatrixGesture::Click,4});
  panel.turn(1);display.update();flush();
  EXPECT_STREQ(label(0),"Menu");EXPECT_STREQ(label(4),"  Lock");
  EXPECT_STREQ(label(5),"> Units");EXPECT_STREQ(label(6),"  Sync");
  EXPECT_NE(display.tft.pixels,boot);
  const auto menu=display.tft.pixels;
  capture(display.tft,"lvgl-menu");

  display.showBootStatus(true);flush();
  EXPECT_STREQ(label(0),"BOOT / FAULT");
  EXPECT_STREQ(label(5),"");EXPECT_NE(display.tft.pixels,menu);
  EXPECT_GT(display.tft.maxFlushPixels,0u);EXPECT_LE(display.tft.maxFlushPixels,320u*8);
  capture(display.tft,"lvgl-fault");
  lv_deinit();
}
