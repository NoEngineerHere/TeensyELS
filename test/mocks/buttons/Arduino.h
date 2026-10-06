#pragma once
#include <array>
#include <cstdint>
#include <clock.h>
using byte = uint8_t;
constexpr int LOW = 0, HIGH = 1, INPUT = 0, INPUT_PULLUP = 2;
namespace ButtonHardwareTest {
inline std::array<int, 256> pins = [] { std::array<int, 256> values{}; values.fill(HIGH); return values; }();
}
inline int digitalRead(uint8_t pin) { return ButtonHardwareTest::pins[pin]; }
inline void pinMode(uint8_t, int) {}
inline unsigned long millis() { return elsMicros() / 1000; }
