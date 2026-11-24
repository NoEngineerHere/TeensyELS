#pragma once
#include <els/hardware_types.h>

constexpr HardwareConfig halfnutWiring() {
  HardwareConfig c;
  c.processor = Processor::Esp32;
  c.motion = {25, 26, 17, 35, 34, 27, true};
  c.matrix = {{32, 33, 2}, {13, 14, 15}};
  c.encoder = {39, 36, EncoderMode::FullQuadrature, 4, true};
  c.indicators = {22, 21, 12};
  return c;
}
