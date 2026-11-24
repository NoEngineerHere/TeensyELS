#pragma once
#include <els/hardware_types.h>

#define ELS_INPUT_LAYOUT ELS_INPUT_LEGACY_MATRIX

constexpr HardwareConfig makeHardwareConfig() {
  HardwareConfig c;
  c.processor = Processor::Teensy41;
  c.motion = {10, 11, 38, 14, 15}; // PCB alarm on pin 39 is unused.
  c.matrix = {{4, 5, 6}, {9, 8, 7}};
  c.encoder = {24, 25, EncoderMode::FullQuadrature, 4, false};
  c.indicators = {41, 40};
  c.oled.sda = 18;
  c.oled.scl = 19;
  return c;
}
