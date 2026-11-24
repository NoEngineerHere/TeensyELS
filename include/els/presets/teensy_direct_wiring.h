#pragma once
#include <els/hardware_types.h>

#define ELS_INPUT_LAYOUT ELS_INPUT_DIRECT

constexpr HardwareConfig teensyDirectWiring() {
  HardwareConfig c;
  c.processor = Processor::Teensy41;
  c.motion = {2, 3, 0, 14, 15};
  c.direct = {4, 5, 6, 7, 8, 9, 10, 24, 25};
  return c;
}
