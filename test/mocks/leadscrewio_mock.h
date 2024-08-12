

#pragma once

class LeadscrewIOMock : public LeadscrewIO {
  uint8_t m_stepPinState;
  uint8_t m_dirPinState;

 public:
  void writeStepPin(uint8_t state) override { m_stepPinState = state; }
  void writeDirPin(uint8_t state) override { m_dirPinState = state; }
  uint8_t readStepPin() override { return m_stepPinState; }
  uint8_t readDirPin() override { return m_dirPinState; }
};