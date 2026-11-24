#pragma once
#include <els/hardware_types.h>

#define ELS_INPUT_LAYOUT ELS_INPUT_LEGACY_MATRIX

constexpr HardwareConfig makeHardwareConfig() {
  HardwareConfig c;
  c.processor = Processor::Esp32;
  c.motion = {25, 26, 17, 37, 36};
  c.matrix = {{32, 33, 2}, {15, 13, 12}};
  c.encoder = {38, 39, EncoderMode::SingleEdge, 1, true};
  c.indicators = {22, 21};
  c.tft = {0, 19, -1, 18, 5, 16, 23, 4};
  return c;
}
