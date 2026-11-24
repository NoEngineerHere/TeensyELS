#pragma once
#include <els/presets/halfnut_wiring.h>

#define ELS_INPUT_LAYOUT ELS_INPUT_MK2_MATRIX

constexpr HardwareConfig makeHardwareConfig() {
  auto c = halfnutWiring();
  c.tft = {0, 19, -1, 18, 5, 16, 23, -1, true, false};
  return c;
}
