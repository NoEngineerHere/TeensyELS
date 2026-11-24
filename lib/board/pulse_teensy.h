#pragma once
#include <leadscrew_io.h>
#include <clock.h>
#ifdef CORE_TEENSY
#include <Arduino.h>
#endif
#if defined(CORE_TEENSY) || defined(ELS_TEST_TEENSY_IO)
template<int StepPin, int DirPin>
class LeadscrewIOTeensy : public LeadscrewIO {
  bool pending = false;
  uint32_t started = 0;
  const uint32_t highUs;
public:
  explicit LeadscrewIOTeensy(PulseTiming timing) : highUs(timing.highUs) {}
  bool begin() override {
    digitalWriteFast(StepPin, LOW);
    pinMode(StepPin, OUTPUT);
    digitalWriteFast(DirPin, LOW);
    pinMode(DirPin, OUTPUT);
    pending = false;
    return true;
  }
  void writeDirPin(uint8_t val) override { digitalWriteFast(DirPin, val); }
  bool startPulse(uint32_t) override {
    if (pending) return false;
    digitalWriteFast(StepPin, HIGH);
    started = elsMicros();
    pending = true;
    return true;
  }
  PulseStatus servicePulse(uint32_t now) override {
    if (!pending) return {PulseResult::IDLE};
    const uint32_t elapsed = now - started;
    if (elapsed < highUs) return {PulseResult::PENDING};
    digitalWriteFast(StepPin, LOW);
    pending = false;
    return {PulseResult::COMPLETE, elsMicros()};
  }
};
#endif
