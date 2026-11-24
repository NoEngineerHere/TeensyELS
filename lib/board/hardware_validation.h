#pragma once
#include <els/hardware_types.h>

namespace hardwareValidation {
// Pinned Teensy 4.1 core_pins.h and ESP-IDF esp32 soc_caps.h. Keep native
// compile probes on the same table; the firmware checks the constants below.
inline constexpr int teensyPinCount = 55;
inline constexpr unsigned long long espInputMask =
    0xffffffffffULL & ~((1ULL << 24) | (15ULL << 28));
inline constexpr unsigned long long espOutputMask = espInputMask & ~(63ULL << 34);

enum class Direction { Input, Output, PullInput };
struct Signal { int pin; Direction direction; bool required = true; };

constexpr bool validGpio(Processor cpu, Signal signal) {
  const int pin = signal.pin;
  if (pin == -1 && !signal.required) return true;
  if (pin < 0) return false;
  if (cpu == Processor::Teensy41) return pin < teensyPinCount;
  if (cpu != Processor::Esp32 || pin >= 40 || (pin >= 6 && pin <= 11)) return false;
  const auto mask = signal.direction == Direction::Input ? espInputMask : espOutputMask;
  return (mask >> pin) & 1;
}

constexpr bool hasRequiredPins(const Signal* pins, unsigned size) {
  for (unsigned i = 0; i < size; ++i)
    if (pins[i].required && pins[i].pin < 0) return false;
  return true;
}

constexpr bool validDirections(Processor cpu, const Signal* pins, unsigned size) {
  for (unsigned i = 0; i < size; ++i)
    if (!validGpio(cpu, pins[i])) return false;
  return true;
}

constexpr bool uniquePins(const Signal* pins, unsigned size) {
  for (unsigned i = 0; i < size; ++i) {
    if (pins[i].pin < 0) continue;
    for (unsigned j = i + 1; j < size; ++j)
      if (pins[i].pin == pins[j].pin) return false;
  }
  return true;
}

constexpr bool validBus(Processor cpu, int bus) {
  return bus >= 0 && bus < (cpu == Processor::Teensy41 ? 3 : 2);
}

constexpr bool i2cRoute(const HardwareConfig& c) {
  if (c.processor != Processor::Teensy41) return true;
  const auto& w = c.oled;
  switch (w.bus) {
    case 0: return w.sda == 18 && w.scl == 19;
    case 1: return (w.sda == 17 || w.sda == 44) && (w.scl == 16 || w.scl == 45);
    case 2: return w.sda == 25 && w.scl == 24;
    default: return false;
  }
}

constexpr bool spiRoute(const HardwareConfig& c) {
  if (c.processor != Processor::Teensy41) return true;
  const auto& s = c.tft;
  switch (s.bus) {
    case 0: return s.mosi == 11 && s.sclk == 13 && s.miso == 12;
    case 1: return s.mosi == 26 && s.sclk == 27 && (s.miso == 1 || s.miso == 39);
    case 2: return (s.mosi == 43 || s.mosi == 50) && (s.sclk == 45 || s.sclk == 49) &&
                   (s.miso == 42 || s.miso == 54);
    default: return false;
  }
}
}
