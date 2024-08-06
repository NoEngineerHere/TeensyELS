#include <config.h>

#pragma once

/**
 * This defines the HW interface for the leadscrew, abstracted away from the
 * actual calls so we can test it more easily
 */
class LeadscrewIO {
 public:
  virtual void writeStepPin(uint8_t val) = 0;
  virtual uint8_t readStepPin() = 0;
  virtual void writeDirPin(uint8_t val) = 0;
  virtual uint8_t readDirPin() = 0;
};
