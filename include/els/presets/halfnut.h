#pragma once
#include "halfnut_wiring.h"

constexpr HardwareConfig makeHardwareConfig() {
  auto c = halfnutWiring();
  c.tft = {0, 19, -1, 18, 5, 16, 23, -1, true, false};
  return c;
}
