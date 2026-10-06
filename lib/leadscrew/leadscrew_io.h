#pragma once
#include <cstdint>
#include <config.h>
class LeadscrewIO {
public:
  enum class PulseResult { IDLE, PENDING, COMPLETE, FAULT };
  virtual bool begin() = 0;
  virtual void writeDirPin(uint8_t val) = 0;
  virtual bool startPulse(uint32_t now) = 0;
  virtual PulseResult servicePulse(uint32_t now) = 0;
};
