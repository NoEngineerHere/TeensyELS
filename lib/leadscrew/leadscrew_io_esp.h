
#include <Wire.h>

#include "leadscrew_io.h"
#pragma once

#if ELS_BOARD == ELS_BOARD_ESP32

class LeadscrewIOESP : public LeadscrewIO {
  inline void writeStepPin(uint8_t val) {
    if (val == 1)
      REG_SET_BIT(GPIO_OUT_REG, ELS_LEADSCREW_STEP_BIT);
    else
      REG_CLR_BIT(GPIO_OUT_REG, ELS_LEADSCREW_STEP_BIT);
    stepBitState = val;
  }
  inline uint8_t readStepPin() { return stepBitState; }

  inline void writeDirPin(uint8_t val) {
    if (val == 1)
      REG_SET_BIT(GPIO_OUT_REG, ELS_LEADSCREW_DIR_BIT);
    else
      REG_CLR_BIT(GPIO_OUT_REG, ELS_LEADSCREW_DIR_BIT);
  }
  inline u_int8_t readDirPin() { return digitalRead(ELS_LEADSCREW_DIR); }
};
#endif                                                                                      