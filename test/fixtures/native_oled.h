#pragma once
#include <els/presets/halfnut_wiring.h>

constexpr HardwareConfig makeHardwareConfig() {
  auto config=halfnutWiring();
  config.oled={1,19,18,0x3d};
  return config;
}
