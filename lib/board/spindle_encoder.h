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
  SpindleEncoder(int pinA, int pinB) : a(pinA), b(pinB) {}
  bool begin();
  int sample(Spindle& spindle);

 private:
  const int a, b;
#if defined(ESP32)
  ESP32Encoder encoder;
  int64_t lastCount;
#elif defined(CORE_TEENSY)
  Encoder encoder;
#endif
};
