#pragma once
#include <cstdint>

struct PulseTiming {
  uint32_t highUs;
  uint32_t lowUs;
  uint32_t dirSetupUs;
  uint32_t dirHoldUs;
};

class LeadscrewIO {
public:
  enum class PulseResult { IDLE, PENDING, COMPLETE, FAULT };
  struct PulseStatus {
    PulseResult result;
    uint32_t lowSince = 0; // Valid only for COMPLETE.
  };
  virtual bool begin() = 0;
  virtual void writeDirPin(uint8_t val) = 0;
  virtual bool startPulse(uint32_t now) = 0;
  virtual PulseStatus servicePulse(uint32_t now) = 0;
};
