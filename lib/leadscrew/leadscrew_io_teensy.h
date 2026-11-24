#pragma once
#include "leadscrew_io.h"
#ifdef CORE_TEENSY
#include <Arduino.h>
#endif
#if defined(CORE_TEENSY) || defined(ELS_TEST_TEENSY_IO)
class LeadscrewIOTeensy : public LeadscrewIO {
  bool pending = false;
  uint32_t started = 0;
public:
  bool begin() override {
    digitalWriteFast(ELS_LEADSCREW_STEP, LOW);
    pinMode(ELS_LEADSCREW_STEP, OUTPUT);
    digitalWriteFast(ELS_LEADSCREW_DIR, LOW);
    pinMode(ELS_LEADSCREW_DIR, OUTPUT);
    pending = false;
    return true;
  }
  void writeDirPin(uint8_t val) override { digitalWriteFast(ELS_LEADSCREW_DIR, val); }
  bool startPulse(uint32_t now) override {
    if (pending) return false;
    digitalWriteFast(ELS_LEADSCREW_STEP, HIGH);
    started = now;
    pending = true;
    return true;
  }
  PulseResult servicePulse(uint32_t now) override {
    if (!pending) return PulseResult::IDLE;
    const uint32_t elapsed = now - started;
    if (elapsed < ELS_STEP_HIGH_US) return PulseResult::PENDING;
    digitalWriteFast(ELS_LEADSCREW_STEP, LOW);
    pending = false;
    return PulseResult::COMPLETE;
  }
};
#endif
