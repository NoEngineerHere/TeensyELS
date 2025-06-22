#pragma once

#include "config.h"

#if ELS_BOARD == ELS_BOARD_TEENSY

#include <Wire.h>

#include "leadscrew_io.h"

class LeadscrewIOTeensy : public LeadscrewIO {
  inline void writeStepPin(uint8_t val) {
    digitalWriteFast(ELS_LEADSCREW_STEP, val);
  }
  inline uint8_t readStepPin() { return digitalReadFast(ELS_LEADSCREW_STEP); }

  inline void writeDirPin(uint8_t val) {
    digitalWriteFast(ELS_LEADSCREW_DIR, val);
  }
  inline u_int8_t readDirPin() { return digitalReadFast(ELS_LEADSCREW_DIR); }
};
#endif
                                                                            