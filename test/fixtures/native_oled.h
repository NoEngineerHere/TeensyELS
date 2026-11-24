#pragma once
#include <els/presets/halfnut_wiring.h>

#define ELS_INPUT_LAYOUT ELS_INPUT_MK2_MATRIX

constexpr HardwareConfig makeHardwareConfig() {
  auto config=halfnutWiring();
  config.oled={1,19,18,0x3d};
  return config;
}
