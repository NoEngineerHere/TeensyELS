#pragma once

#if defined(ESP32)
#include <ESP32Encoder.h>
#elif defined(CORE_TEENSY)
#include <Encoder.h>
#endif
#include <cstdint>

class Spindle;

class SpindleEncoder {
 public:
  SpindleEncoder(int pinA, int pinB);
  void sample(Spindle& spindle);

 private:
#if defined(ESP32)
  ESP32Encoder encoder;
  int64_t lastCount;
#elif defined(CORE_TEENSY)
  Encoder encoder;
#endif
};
