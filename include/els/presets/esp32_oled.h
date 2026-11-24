#pragma once
#include <els/presets/halfnut_wiring.h>

#define ELS_INPUT_LAYOUT ELS_INPUT_MK2_MATRIX

constexpr HardwareConfig makeHardwareConfig() {
  auto c = halfnutWiring();
  c.oled.sda = 19;
  c.oled.scl = 18;
  return c;
}
