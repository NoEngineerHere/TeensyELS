#pragma once
#ifdef PROBE_ESP_OUTPUT
#include <els/presets/halfnut_wiring.h>
#define ELS_INPUT_LAYOUT ELS_INPUT_MK2_MATRIX
constexpr HardwareConfig makeHardwareConfig() {
  auto c=halfnutWiring();
  c.motion.step=34;
  c.motion.spindleB=37;
  c.oled={0,19,18};
  return c;
}
#else
#include <els/presets/teensy_direct_wiring.h>
#define ELS_INPUT_LAYOUT ELS_INPUT_DIRECT
constexpr HardwareConfig makeHardwareConfig() {
  auto c=teensyDirectWiring();
  c.tft={0,11,12,13,20,21,22,23};
#ifdef PROBE_COLLISION
  c.tft.cs=c.motion.step;
#else
  c.tft.mosi=19;
#endif
  return c;
}
#endif
