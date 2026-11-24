#pragma once
#include "teensy_direct_wiring.h"

constexpr HardwareConfig makeHardwareConfig() {
  auto c = teensyDirectWiring();
  c.tft = {0, 11, 12, 13, 20, 21, 22, 23, true, false};
  return c;
}
