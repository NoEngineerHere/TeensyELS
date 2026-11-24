#pragma once
#include <els/presets/teensy_direct_wiring.h>

#define ELS_INPUT_LAYOUT ELS_INPUT_DIRECT

constexpr HardwareConfig makeHardwareConfig() {
  auto c = teensyDirectWiring();
  c.oled.sda = 18;
  c.oled.scl = 19;
  return c;
}
