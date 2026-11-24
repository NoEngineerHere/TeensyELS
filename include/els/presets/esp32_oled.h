#pragma once
#include "halfnut_wiring.h"

constexpr HardwareConfig makeHardwareConfig() {
  auto c = halfnutWiring();
  c.oled.sda = 19;
  c.oled.scl = 18;
  return c;
}
