#pragma once
#include <els/hardware_types.h>

constexpr HardwareConfig teensyDirectWiring() {
  HardwareConfig c;
  c.processor = Processor::Teensy41;
  c.motion = {2, 3, 0, 14, 15};
  c.direct = {4, 5, 6, 7, 8, 9, 10, 24, 25};
  return c;
}
