
#include <Wire.h>

#include "leadscrew_io.h"
#pragma once

class LeadscrewIOImpl : public LeadscrewIO {
  inline void writeStepPin(uint8_t val) {
    digitalWriteFast(ELS_LEADSCREW_STEP, val);
  }
  inline uint8_t readStepPin() { return digitalReadFast(ELS_LEADSCREW_STEP); }

  inline void writeDirPin(uint8_t val) {
    digitalWriteFast(ELS_LEADSCREW_DIR, val);
  }
  inline u_int8_t readDirPin() { return digitalReadFast(ELS_LEADSCREW_DIR); }
};