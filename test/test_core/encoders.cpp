#include <gtest/gtest.h>
#include <spindle.h>
#include <ESP32Encoder.h>

#undef ELS_HARDWARE_CONFIG
#define ELS_HARDWARE_CONFIG "native_oled.h"
#define ESP32
#include "../../lib/board/spindle_esp32.cpp"
namespace EspEncoderSurface {
#include "../../lib/board/ui_encoder.cpp"
}
#undef ESP32

TEST(EspEncoders, ConstructionDoesNotAttachAndFailedAttachmentsDoNotReadPcnt) {
  using namespace EncoderHardwareTest;
  attaches=reads=0;attachSucceeds=false;
  SpindleEncoder spindle(EspEncoderSurface::hardware.motion.spindleA,EspEncoderSurface::hardware.motion.spindleB);
  EspEncoderSurface::UiEncoder ui;
  EXPECT_EQ(attaches,0);EXPECT_EQ(reads,0);
  EXPECT_FALSE(spindle.begin());EXPECT_FALSE(ui.begin());
  EXPECT_EQ(attaches,2);EXPECT_EQ(reads,0);
}

TEST(EspEncoders, UsesExternalPullsAndBeginsFromTheAttachedCountBaseline) {
  using namespace EncoderHardwareTest;
  attachSucceeds=true;count=700;
  SpindleEncoder source(EspEncoderSurface::hardware.motion.spindleA,EspEncoderSurface::hardware.motion.spindleB);
  EspEncoderSurface::UiEncoder ui;Spindle spindle;
  ASSERT_TRUE(source.begin());
  EXPECT_EQ(ESP32Encoder::useInternalWeakPullResistors,puType::none);
  ESP32Encoder::useInternalWeakPullResistors=puType::up;
  ASSERT_TRUE(ui.begin());
  EXPECT_EQ(ESP32Encoder::useInternalWeakPullResistors,puType::none);
  source.sample(spindle);EXPECT_EQ(spindle.consumePosition(),0);
  EXPECT_EQ(ui.poll(),0);
  count+=3;source.sample(spindle);EXPECT_EQ(spindle.consumePosition(),3);EXPECT_EQ(ui.poll(),0);
  ++count;EXPECT_EQ(ui.poll(),1);
}
