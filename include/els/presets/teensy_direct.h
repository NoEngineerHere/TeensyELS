#pragma once
#include "teensy_direct_wiring.h"

constexpr HardwareConfig makeHardwareConfig() {
  auto c = teensyDirectWiring();
  c.oled.sda = 18;
  c.oled.scl = 19;
  return c;
}
