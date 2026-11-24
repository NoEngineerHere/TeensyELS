#pragma once
#include <els/presets/teensy_direct_wiring.h>

#define ELS_INPUT_LAYOUT ELS_INPUT_DIRECT

constexpr HardwareConfig makeHardwareConfig() {
  auto c = teensyDirectWiring();
  c.tft = {0, 11, 12, 13, 20, 21, 22, 23, true, false};
  return c;
}
